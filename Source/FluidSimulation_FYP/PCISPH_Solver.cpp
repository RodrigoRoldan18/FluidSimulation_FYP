// Fill out your copyright notice in the Description page of Project Settings.


#include "PCISPH_Solver.h"
#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "BCCLatticePointsGenerator.h"
#include "Kernels.h"

double APCISPH_Solver::computeDelta(double timeStepInSeconds)
{
	const double kernelRadius = m_gameMode->GetKernelRadius();
	TArray<FVector> points;
	FVector origin = FVector(0.0);

	FVector lowercorner = FVector(0.0);
	FVector uppercorner = FVector(0.0);
	lowercorner -= FVector(1.5 * kernelRadius);
	uppercorner += FVector(1.5 * kernelRadius);

	BCCLatticePointsGenerator pointsGenerator; //find a way to generate points in the body-centered cubic pattern. 
	//(This pattern has one point in the center of the unit cube and eight corner points.)

	pointsGenerator.generate(lowercorner, uppercorner, m_gameMode->GetTargetSpacing(), &points);

	FSphSpikyKernel kernel(kernelRadius);
	double denom = 0;
	FVector denom1;
	double denom2 = 0;

	for (size_t i = 0; i < points.Num(); ++i)
	{
		const FVector& point = points[i];
		double distanceSquared = point.SizeSquared();

		if (distanceSquared < kernelRadius * kernelRadius)
		{
			double distance = FMath::Sqrt(distanceSquared);
			FVector direction = (distance > 0.0) ? point / distance : FVector();

			//grad(Wij)
			FVector gradWij = kernel.Gradient(distance, direction);
			denom1 += gradWij;
			denom2 += FVector::DotProduct(gradWij, gradWij);
		}
	}

	denom += FVector::DotProduct(-denom1, denom1) - denom2;

	return (FMath::Abs(denom) > 0.0) ? -1 / (computeBeta(timeStepInSeconds) * denom) : 0;
}

double APCISPH_Solver::computeBeta(double timeStepInSeconds)
{
	return 2.0 * FMath::Square((*m_ptrParticles)[0]->kMass * timeStepInSeconds / m_gameMode->GetTargetDensity());
}

void APCISPH_Solver::computePressureGradientForce(double timeStepInSeconds, const TArray<double>& densities)
{
	//do the accumulatepressureforce function here
	size_t n = m_ptrParticles->Num();

	const double massSquared = (*m_ptrParticles)[0]->kMass * (*m_ptrParticles)[0]->kMass;
	const FSphSpikyKernel kernel(m_gameMode->GetKernelRadius());

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		const auto& neighbours = (*m_gameMode->GetNeighbourLists())[i];
		for (size_t j : neighbours)
		{
			double dist = FVector::Distance((*m_ptrParticles)[i]->GetParticlePosition(), (*m_ptrParticles)[j]->GetParticlePosition());
			if (dist > 0.0)
			{
				FVector dir = ((*m_ptrParticles)[j]->GetParticlePosition() - (*m_ptrParticles)[i]->GetParticlePosition()) / dist;
				FVector pressureForceResult = m_tempPressureForces[i] - massSquared *
					((*m_ptrParticles)[i]->GetParticlePressure() / (densities[i] * densities[i]) +
						(*m_ptrParticles)[j]->GetParticlePressure() / (densities[j] * densities[j])) *
					kernel.Gradient(dist, dir);
				Mutex.Lock();
				m_tempPressureForces[i] = pressureForceResult;
				Mutex.Unlock();
			}
		}
		/*if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict pressure FORCE: %s"), *m_tempPressureForces[i].ToString());
		}*/
		});
}

void APCISPH_Solver::onBeginAdvanceTimeStep()
{
	AParticleSystemSolver::onBeginAdvanceTimeStep();

	size_t n = m_ptrParticles->Num();

	//Initialise buffers
	m_tempPositions.Reserve(n);
	m_tempVelocities.Reserve(n);
	m_tempPressureForces.Reserve(n);
	m_densityErrors.Reserve(n);

	m_tempPositions.SetNumZeroed(n);
	m_tempVelocities.SetNumZeroed(n);
	m_tempPressureForces.SetNumZeroed(n);
	m_densityErrors.SetNumZeroed(n);
}

void APCISPH_Solver::accumulatePressureForce(double timeStepInSeconds)
{
	const size_t n = m_ptrParticles->Num();
	const double targetDensity = m_gameMode->GetTargetDensity();
	const double mass = (*m_ptrParticles)[0]->kMass;
	const double delta = computeDelta(timeStepInSeconds); //the scalar maps the density to the optimal pressure that cancels out density error.
	//UE_LOG(LogTemp, Warning, TEXT("delta: %f"), delta);
	//Predicted density ds
	TArray<double> ds;
	FSphStdKernel kernel(m_gameMode->GetKernelRadius());

	//Initialise buffers
	ds.Reserve(n);
	ds.SetNumZeroed(n);

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticlePressure(0.0);
		ds[i] = (*m_ptrParticles)[i]->GetParticleDensity();
		Mutex.Unlock();
		});

	unsigned int maxNumberIter = 0;
	double maxDensityError = 0.0;
	double densityErrorRatio = 0.0;

	for (unsigned int k = 0; k < m_maxNumberOfIterations; ++k)
	{
		//Predict velocity and position (perform time integration from the current state to the temp state)
		ParallelFor(n, [&](size_t i) {
			FVector predictVel = (*m_ptrParticles)[i]->GetParticleVelocity() + timeStepInSeconds / mass *
				((*m_ptrParticles)[i]->GetParticleForce() + m_tempPressureForces[i]);

			FVector predictPos = (*m_ptrParticles)[i]->GetParticlePosition() + timeStepInSeconds * predictVel;

			Mutex.Lock();
			m_tempVelocities[i] = predictVel;
			m_tempPositions[i] = predictPos;
			Mutex.Unlock();

			/*if (i == 723)
			{
				UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict VELOCITY: %s"), *predictVel.ToString());
				UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict POSITION: %s"), *predictPos.ToString());
			}*/
			});

		//Resolve collisions. DISABLED THIS FOR NOW. IT'S CAUSING FREEZE
		//resolveCollision(&m_tempPositions, &m_tempVelocities); 

		//Compute pressure from density error
		ParallelFor(n, [&](size_t i) {
			double weightSum = 0.0;
			const auto& neighbours = (*m_gameMode->GetNeighbourLists())[i];
			for (size_t j : neighbours)
			{
				double dist = FVector::Distance(m_tempPositions[j], m_tempPositions[i]);
				weightSum += kernel(dist);
			}
			weightSum += kernel(0);

			double density = mass * weightSum;
			double densityError = (density - targetDensity);
			double pressure = delta * densityError;

			if (pressure < 0.0)
			{
				pressure *= m_negaitvePressureScale;
				densityError *= m_negaitvePressureScale;
			}
			double newParticlePressure = (*m_ptrParticles)[i]->GetParticlePressure() + pressure;
			newParticlePressure /= 100.0; //PRESSURE VALUE IS TOO HIGH!!!!! THIS IS A HACK FIX!!! 

			Mutex.Lock();
			(*m_ptrParticles)[i]->SetParticlePressure(newParticlePressure);
			ds[i] = density;
			m_densityErrors[i] = densityError;
			Mutex.Unlock();

			/*if (i == 723)
			{
				UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict DENSITY: %f"), density);
				UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict DENSITY ERROR: %f"), densityError);
				UE_LOG(LogTemp, Warning, TEXT("Particle 723 predict PRESSURE: %f"), newParticlePressure);				
			}*/
			});

		//Compute pressure gradient force
		m_tempPressureForces.SetNumZeroed(n);
		computePressureGradientForce(timeStepInSeconds, ds);

		//Compute max density error
		for (size_t i = 0; i < n; ++i)
		{
			maxDensityError = FMath::Abs(maxDensityError) > FMath::Abs(m_densityErrors[i]) ? FMath::Abs(maxDensityError) : FMath::Abs(m_densityErrors[i]);
		}

		densityErrorRatio = maxDensityError / targetDensity;
		maxNumberIter = k + 1;

		if (FMath::Abs(densityErrorRatio) < m_maxDensityErrorRatio)
		{
			break;
		}
	}

	/*UE_LOG(LogTemp, Warning, TEXT("Number of PCI iterations: %i"), maxNumberIter);
	UE_LOG(LogTemp, Warning, TEXT("Max density error after PCI iteration: %f"), maxDensityError);
	if (FMath::Abs(densityErrorRatio) > m_maxDensityErrorRatio)
	{
		UE_LOG(LogTemp, Warning, TEXT("Max density error ration is greater than the threshold!"));
		UE_LOG(LogTemp, Warning, TEXT("Ratio: %f, Threshold: %f"), densityErrorRatio, m_maxDensityErrorRatio);
	}*/

	//Accumulate pressure force
	ParallelFor(n, [&](size_t i) {
		FVector newPressureForce = (*m_ptrParticles)[i]->GetParticleForce() + m_tempPressureForces[i];

		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticleForce(newPressureForce);
		Mutex.Unlock();
		/*if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 compute pressure FORCE: %s"), *newPressureForce.ToString());
		}*/
		});
}

void APCISPH_Solver::accumulateForces(double timeStepInSeconds)
{
	//STAGE 5
	accumulateExternalForces(timeStepInSeconds);
	//STAGE 4
	accumulateNonPressureForces(timeStepInSeconds);
	//STAGE 2 & 3	
	accumulatePressureForce(timeStepInSeconds);
}
