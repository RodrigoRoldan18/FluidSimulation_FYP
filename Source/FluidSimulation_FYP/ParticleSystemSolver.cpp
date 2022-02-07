// Fill out your copyright notice in the Description page of Project Settings.


#include "ParticleSystemSolver.h"
#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"

void AParticleSystemSolver::beginAdvanceTimeStep()
{
	//Allocate buffers
	size_t n = m_gameMode->GetNumberOfParticles();
	m_newPositions.Reserve(n);
	m_newVelocities.Reserve(n);

	//Clear forces
}

void AParticleSystemSolver::endAdvanceTimeStep()
{
	//Update data
	size_t n = m_gameMode->GetNumberOfParticles();
	TArray<AFluidParticle*>* ptrParticleArray = m_gameMode->GetParticleArrayPtr();

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		Mutex.Lock();
		(*ptrParticleArray)[i]->SetParticlePosition(m_newPositions[i]);
		(*ptrParticleArray)[i]->SetParticleVelocity(m_newVelocities[i]);
		Mutex.Unlock();
		});
}

void AParticleSystemSolver::timeIntegration(double timeIntervalInSeconds)
{
	size_t n = m_gameMode->GetNumberOfParticles();
	TArray<AFluidParticle*>* ptrParticleArray = m_gameMode->GetParticleArrayPtr();

	ParallelFor(n, [&](size_t i) {
		//Integrate velocity first
		FVector& newVelocity = m_newVelocities[i];
		newVelocity =(*ptrParticleArray)[i]->GetParticleVelocity() + timeIntervalInSeconds *
			(*ptrParticleArray)[i]->GetParticleForce() / (*ptrParticleArray)[i]->GetParticleMass();

		//Integrate position.
		FVector& newPosition = m_newPositions[i];
		newPosition = (*ptrParticleArray)[i]->GetParticlePosition() + timeIntervalInSeconds * newVelocity;
		});
}

// Sets default values for this component's properties
AParticleSystemSolver::AParticleSystemSolver()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryActorTick.bCanEverTick = false;

	m_gameMode = StaticCast<AFluidSimulation_FYPGameModeBase*>(UGameplayStatics::GetGameMode(GetWorld()));
}


// Called when the game starts
void AParticleSystemSolver::BeginPlay()
{
	Super::BeginPlay();	
}


void AParticleSystemSolver::OnAdvanceTimeStep(double timeIntervalInSeconds)
{
	beginAdvanceTimeStep();

	accumulateForces(timeIntervalInSeconds);
	timeIntegration(timeIntervalInSeconds);
	resolveCollision();

	endAdvanceTimeStep();
}

void AParticleSystemSolver::accumulateForces(double timeStepInSeconds)
{
	accumulateExternalForces();
}

void AParticleSystemSolver::accumulateExternalForces()
{
	size_t n = m_gameMode->GetNumberOfParticles();
	TArray<AFluidParticle*>* ptrParticleArray = m_gameMode->GetParticleArrayPtr();

	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		//Gravity
		FVector force = (*ptrParticleArray)[i]->GetParticleMass() * m_gravity;

		//Wind forces
		FVector relativeVelocity = (*ptrParticleArray)[i]->GetParticleVelocity(); //sample the wind to the particle position

		force += -m_dragCoefficient * relativeVelocity;

		Mutex.Lock();
		(*ptrParticleArray)[i]->SetParticleForce((*ptrParticleArray)[i]->GetParticleForce() + force);
		Mutex.Unlock();
		});
}

void AParticleSystemSolver::resolveCollision()
{
	size_t n = m_gameMode->GetNumberOfParticles();
	TArray<AFluidParticle*>* ptrParticleArray = m_gameMode->GetParticleArrayPtr();
	const float kParticleRadius = m_gameMode->GetParticleRadius();

	ParallelFor(n, [&](size_t i) {
		//resolve collision
		});
}
