// Fill out your copyright notice in the Description page of Project Settings.


#include "ParticleSystemSolver.h"
#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "Kernels.h"
#include "Collider.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"

void AParticleSystemSolver::beginAdvanceTimeStep()
{
	//Allocate buffers
	size_t n = m_ptrParticles->Num();
	m_newPositions.Reserve(n);
	m_newVelocities.Reserve(n);
	m_newPositions.SetNumZeroed(n);
	m_newVelocities.SetNumZeroed(n);

	//Clear forces
	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticleForce(FVector(0.0f));
		Mutex.Unlock();
		});

	//need to update density prior to any SPH operations
	if (m_gameMode == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("game mode pointer isn't initialised"));
		return;
	}

	//STAGE 1 - MEASURE DENSITY WITH PARTICLES' CURRENT LOCATIONS
	m_gameMode->BuildNeighbourSearcher();
	m_gameMode->BuildNeighbourLists();
	m_gameMode->UpdateDensities();
}

void AParticleSystemSolver::endAdvanceTimeStep(double timeIntervalInSeconds)
{
	//Update data
	size_t n = m_ptrParticles->Num();

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticlePosition(m_newPositions[i]);
		(*m_ptrParticles)[i]->SetActorLocation(m_newPositions[i]);
		(*m_ptrParticles)[i]->SetParticleVelocity(m_newVelocities[i]);
		Mutex.Unlock();
		});

	//this will dampen any noticeable noises (DISABLED FOR NOW BECAUSE IT'S CAUSING ISSUES)
	//computePseudoViscosity(timeIntervalInSeconds);
}

void AParticleSystemSolver::timeIntegration(double timeIntervalInSeconds)
{
	//STAGE 6 - PERFORM TIME INTEGRATION

	size_t n = m_ptrParticles->Num();

	ParallelFor(n, [&](size_t i) {
		//Integrate velocity first
		FVector& newVelocity = m_newVelocities[i];
		newVelocity = (*m_ptrParticles)[i]->GetParticleVelocity() + timeIntervalInSeconds *
			(*m_ptrParticles)[i]->GetParticleForce() / (*m_ptrParticles)[i]->kMass;
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 NEW VELOCITY: %s"), *newVelocity.ToString());
		}

		//Integrate position.
		FVector& newPosition = m_newPositions[i];
		newPosition = (*m_ptrParticles)[i]->GetParticlePosition() + timeIntervalInSeconds * newVelocity;
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 NEW POSITION: %s"), *(*m_ptrParticles)[i]->GetParticlePosition().ToString());
		}
		});
}

// Sets default values for this component's properties
AParticleSystemSolver::AParticleSystemSolver()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = false;
}

void AParticleSystemSolver::initPhysicsSolver(TArray<AFluidParticle*>* ptrParticles, AFluidSimulation_FYPGameModeBase* gameMode)
{
	m_ptrParticles = ptrParticles;
	m_gameMode = gameMode;

	TArray<AActor*> foundColliders;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACollider::StaticClass(), foundColliders);
	m_colliders.Reserve(foundColliders.Num());
	for (AActor* a : foundColliders)
	{
		ACollider* castCollider = Cast<ACollider>(a);
		m_colliders.Push(castCollider);
	}
	UE_LOG(LogTemp, Warning, TEXT("we have %i colliders in the world"), m_colliders.Num());
}

void AParticleSystemSolver::OnAdvanceTimeStep(double timeIntervalInSeconds)
{
	if (m_ptrParticles == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("particles pointer isn't initialised"));
		return;
	}
	beginAdvanceTimeStep();

	accumulateForces(timeIntervalInSeconds);
	timeIntegration(timeIntervalInSeconds);
	
	//Resolve collisions disabled until fully implemented
	resolveCollision();

	endAdvanceTimeStep(timeIntervalInSeconds);
}

const FVector AParticleSystemSolver::SampleVectorField(const FVector& _subject, const FVector& _vectorField) const
{
	//this uses radians
	return FVector(FMath::Sin(_subject.X) * FMath::Sin(_vectorField.Z),
		FMath::Sin(_subject.Z) * FMath::Sin(_vectorField.Y),
		FMath::Sin(_subject.Y) * FMath::Sin(_vectorField.X));

	/*return FVector(UKismetMathLibrary::DegSin(_subject.X) * UKismetMathLibrary::DegSin(_vectorField.Y),
		UKismetMathLibrary::DegSin(_subject.Y) * UKismetMathLibrary::DegSin(_vectorField.Z),
		UKismetMathLibrary::DegSin(_subject.Z) * UKismetMathLibrary::DegSin(_vectorField.X));*/
}

const double AParticleSystemSolver::SampleScalarField(const FVector& _subject) const
{
	return FMath::Sin(_subject.X) * FMath::Sin(_subject.Y) * FMath::Sin(_subject.Z);
}

void AParticleSystemSolver::accumulateForces(double timeStepInSeconds)
{
	//STAGE 2 & 3
	accumulatePressureForce(timeStepInSeconds);
	//STAGE 4
	accumulateNonPressureForces(timeStepInSeconds);
	//STAGE 5
	accumulateExternalForces(timeStepInSeconds);
}

void AParticleSystemSolver::accumulateExternalForces(double timeStepInSeconds)
{
	//STAGE 5 - COMPUTE THE GRAVITY AND OTHER EXTERNAL FORCES
	size_t n = m_ptrParticles->Num();

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		//Gravity
		FVector force = (*m_ptrParticles)[i]->kMass * m_kGravity;

		//Wind forces
		FVector sampleVectorFieldResult = SampleVectorField((*m_ptrParticles)[i]->GetParticlePosition(), m_kWind);

		/*if (i == 3)
		{
			UE_LOG(LogTemp, Warning, TEXT("The vector field result for particle %i is x: %f y: %f z: %f"), i, sampleVectorFieldResult.X, sampleVectorFieldResult.Y, sampleVectorFieldResult.Z);
		}*/
		FVector relativeVelocity = (*m_ptrParticles)[i]->GetParticleVelocity() - sampleVectorFieldResult;

		force += -m_dragCoefficient * relativeVelocity;

		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticleForce((*m_ptrParticles)[i]->GetParticleForce() + force);
		Mutex.Unlock();
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 external FORCE: %s"), *(*m_ptrParticles)[i]->GetParticleForce().ToString());
		}
		});
}

void AParticleSystemSolver::accumulateNonPressureForces(double timeStepInSeconds)
{
	accumulateViscosityForce();
}

void AParticleSystemSolver::accumulatePressureForce(double timeStepInSeconds)
{
	computePressure();

	//STAGE 3 - COMPUTE THE GRADIENT PRESSURE FORCE

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
				FVector pressureForceResult = (*m_ptrParticles)[i]->GetParticleForce() - massSquared *
					((*m_ptrParticles)[i]->GetParticlePressure() / ((*m_ptrParticles)[i]->GetParticleDensity() * (*m_ptrParticles)[i]->GetParticleDensity()) +
					(*m_ptrParticles)[j]->GetParticlePressure() / ((*m_ptrParticles)[j]->GetParticleDensity() * (*m_ptrParticles)[j]->GetParticleDensity())) *
					kernel.Gradient(dist, dir);
				Mutex.Lock();
				(*m_ptrParticles)[i]->SetParticleForce(pressureForceResult);
				Mutex.Unlock();				
			}
		}
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 pressure FORCE: %s"), *(*m_ptrParticles)[i]->GetParticleForce().ToString());
		}
		});
}

void AParticleSystemSolver::computePressure()
{
	//STAGE 2 - COMPUTE THE PRESSURE BASED ON THE DENSITY
	size_t n = m_ptrParticles->Num();
	const double targetDensity = m_gameMode->GetTargetDensity();
	const double eosScale = targetDensity * (m_speedOfSound * m_speedOfSound);
	
	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		double pressure = computePressureFromEOS((*m_ptrParticles)[i]->GetParticleDensity(), targetDensity, eosScale, m_eosExponent, m_negaitvePressureScale);
		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticlePressure(pressure);
		Mutex.Unlock();
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 PRESSURE: %f"), (*m_ptrParticles)[i]->GetParticlePressure());
		}
		});
}

void AParticleSystemSolver::accumulateViscosityForce()
{
	//STAGE 4 - COMPUTE THE VISCOSITY FORCE
	size_t n = m_ptrParticles->Num();

	const double massSquared = (*m_ptrParticles)[0]->kMass * (*m_ptrParticles)[0]->kMass;
	const FSphSpikyKernel kernel(m_gameMode->GetKernelRadius());

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		const auto& neighbours = (*m_gameMode->GetNeighbourLists())[i];
		for (size_t j : neighbours)
		{
			double dist = FVector::Distance((*m_ptrParticles)[i]->GetParticlePosition(), (*m_ptrParticles)[j]->GetParticlePosition());
			FVector viscosityForceResult = (*m_ptrParticles)[i]->GetParticleForce() + m_viscosityCoefficient * massSquared *
				((*m_ptrParticles)[j]->GetParticleVelocity() - (*m_ptrParticles)[i]->GetParticleVelocity()) / (*m_ptrParticles)[j]->GetParticleDensity() *
				kernel.SecondDerivative(dist);
			Mutex.Lock();
			(*m_ptrParticles)[i]->SetParticleForce(viscosityForceResult);
			Mutex.Unlock();			
		}
		if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 viscosity FORCE: %s"), *(*m_ptrParticles)[i]->GetParticleForce().ToString());
		}
		});
}

void AParticleSystemSolver::computePseudoViscosity(double timeStepInSeconds)
{
	size_t n = m_ptrParticles->Num();
	const double mass = (*m_ptrParticles)[0]->kMass;
	const FSphSpikyKernel kernel(m_gameMode->GetKernelRadius());
	TArray<FVector> smoothedVelocities;
	smoothedVelocities.Reserve(n);
	smoothedVelocities.SetNumZeroed(n);

	ParallelFor(n, [&](size_t i) {
		double weightSum = 0.0f;
		FVector smoothedVelocity;

		const auto& neighbours = (*m_gameMode->GetNeighbourLists())[i];
		for (size_t j : neighbours)
		{
			double dist = FVector::Distance((*m_ptrParticles)[i]->GetParticlePosition(), (*m_ptrParticles)[j]->GetParticlePosition());
			double wj = mass / (*m_ptrParticles)[j]->GetParticleDensity() * kernel(dist);
			weightSum += wj;
			smoothedVelocity += wj * (*m_ptrParticles)[j]->GetParticleVelocity();
		}

		double wi = mass / (*m_ptrParticles)[i]->GetParticleDensity();
		weightSum += wi;
		smoothedVelocity += wi * (*m_ptrParticles)[i]->GetParticleVelocity();

		if (weightSum > 0.0)
		{
			smoothedVelocity /= weightSum;
		}

		smoothedVelocities[i] = smoothedVelocity;
		});

	double factor = timeStepInSeconds * m_pseudoViscossityCoefficient;
	factor = FMath::Clamp(factor, 0.0, 1.0);

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		FVector newVelocity = FMath::Lerp((*m_ptrParticles)[i]->GetParticleVelocity(), smoothedVelocities[i], factor);
		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticleVelocity(newVelocity);
		Mutex.Unlock();
		});
}

double AParticleSystemSolver::computePressureFromEOS(double density, double targetDensity, double eosScale, double eosExponent, double negativePressureScale)
{
	double p = eosScale / eosExponent * (FMath::Pow((density / targetDensity), eosExponent) - 1.0); //THIS IS GIVING A NEGATIVE SOLUTION. Maybe the density is too low. Lowered the target density to compensate.

	//Negative pressure scaling
	if (p < 0)
	{
		p *= negativePressureScale;
	}

	return p;
}

void AParticleSystemSolver::resolveCollision()
{
	//whitebox function, we will get to external collisions later

	size_t n = m_ptrParticles->Num();
	ParallelFor(n, [&](size_t i) {
		for (ACollider* c : m_colliders)
		{
			if (c != nullptr)
			{
				const float kParticleRadius = (*m_ptrParticles)[0]->kRadius;
				c->ResolveCollision(m_newPositions[i], m_newVelocities[i], kParticleRadius, m_restitutionCoefficient, &m_newPositions[i], &m_newVelocities[i]);
			}
		}		
	});	
}
