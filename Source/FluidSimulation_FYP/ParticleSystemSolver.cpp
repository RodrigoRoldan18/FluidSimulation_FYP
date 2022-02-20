// Fill out your copyright notice in the Description page of Project Settings.


#include "ParticleSystemSolver.h"
#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
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
}

void AParticleSystemSolver::endAdvanceTimeStep()
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
}

void AParticleSystemSolver::timeIntegration(double timeIntervalInSeconds)
{
	size_t n = m_ptrParticles->Num();

	ParallelFor(n, [&](size_t i) {
		//Integrate velocity first
		FVector& newVelocity = m_newVelocities[i];
		newVelocity = (*m_ptrParticles)[i]->GetParticleVelocity() + timeIntervalInSeconds *
			(*m_ptrParticles)[i]->GetParticleForce() / (*m_ptrParticles)[i]->kMass;

		//Integrate position.
		FVector& newPosition = m_newPositions[i];
		newPosition = (*m_ptrParticles)[i]->GetParticlePosition() + timeIntervalInSeconds * newVelocity;
		});
}

// Sets default values for this component's properties
AParticleSystemSolver::AParticleSystemSolver()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = false;
}

void AParticleSystemSolver::initPhysicsSolver(TArray<AFluidParticle*>* ptrParticles)
{
	m_ptrParticles = ptrParticles;
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
	resolveCollision();

	endAdvanceTimeStep();
}

const FVector AParticleSystemSolver::SampleVectorField(const FVector& _subject, const FVector& _vectorField) const
{
	//this uses radians
	return FVector(FMath::Sin(_subject.X) * FMath::Sin(_vectorField.Y),
		FMath::Sin(_subject.Y) * FMath::Sin(_vectorField.Z),
		FMath::Sin(_subject.Z) * FMath::Sin(_vectorField.Z));

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
	accumulateExternalForces(timeStepInSeconds);
}

void AParticleSystemSolver::accumulateExternalForces(double timeStepInSeconds)
{
	size_t n = m_ptrParticles->Num();

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		//Gravity
		FVector force = (*m_ptrParticles)[i]->kMass * m_kGravity;

		//Wind forces
		FVector sampleVectorFieldResult = SampleVectorField((*m_ptrParticles)[i]->GetParticlePosition(), m_kWind);

		if (i == 3)
		{
			UE_LOG(LogTemp, Warning, TEXT("The vector field result for particle %i is x: %f y: %f z: %f"), i, sampleVectorFieldResult.X, sampleVectorFieldResult.Y, sampleVectorFieldResult.Z);
		}		
		FVector relativeVelocity = (*m_ptrParticles)[i]->GetParticleVelocity() - sampleVectorFieldResult;

		force += -m_dragCoefficient * relativeVelocity;

		Mutex.Lock();
		(*m_ptrParticles)[i]->SetParticleForce((*m_ptrParticles)[i]->GetParticleForce() + force);
		Mutex.Unlock();
		});
}

void AParticleSystemSolver::resolveCollision()
{
	//whitebox function, we will get to external collisions later

	if (m_collider != nullptr)
	{
		size_t n = m_ptrParticles->Num();
		const float kParticleRadius = (*m_ptrParticles)[0]->kRadius;

		ParallelFor(n, [&](size_t i) {
			//resolve collision
			//m_collider->ResolveCollision(m_newPositions[i], m_newVelocities[i], kParticleRadius, m_restitutionCoefficient, &m_newPositions[i], &m_newVelocities[i]);
			});
	}
}
