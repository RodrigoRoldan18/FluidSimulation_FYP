// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParticleSystemSolver.generated.h"

UCLASS()
class FLUIDSIMULATION_FYP_API AParticleSystemSolver : public AActor
{
	GENERATED_BODY()

	//these two functions are the pre- and postprocessing functions.
	void beginAdvanceTimeStep();
	void endAdvanceTimeStep();

	void timeIntegration(double timeIntervalInSeconds);

	FVector m_wind{ FVector(0.0f, -2.0f, 0.0f) }; //test wind 
	FVector m_gravity{ FVector(0.0f, 0.0f, -9.8f) };
	double m_dragCoefficient = 1e-4;

	TArray<class AFluidParticle*>* m_ptrParticles;

	//these are needed for post processing
	TArray<FVector> m_newPositions;
	TArray<FVector> m_newVelocities;

public:	
	// Sets default values for this component's properties
	AParticleSystemSolver();
	~AParticleSystemSolver() = default;

	void initPhysicsSolver(TArray<class AFluidParticle*>* ptrParticles);
	void OnAdvanceTimeStep(double timeIntervalInSeconds);

protected:
	void accumulateForces(double timeStepInSeconds);
	void accumulateExternalForces(double timeStepInSeconds);

	//only external forces will be taken into account here.
	void resolveCollision();		
};