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
	void endAdvanceTimeStep(double timeIntervalInSeconds);

	void timeIntegration(double timeIntervalInSeconds);

	const FVector m_kWind{ FVector(10.0f, 25.0f, 0.0f) }; //test wind (vector field)
	const FVector m_kGravity{ FVector(0.0f, 0.0f, -9.8f) };
	double m_restitutionCoefficient{ 0.0 };
	double m_dragCoefficient{ 0.001 }; //1e-4 TEMPORARY HACK FIX

	//exponent component of EOS(Tait's equation)
	double m_eosExponent{ 1.0 }; //Becker and Teschner suggest 7 which is stiffer(will apply higher pressure for the same density offset). Muller suggests 1
	double m_viscosityCoefficient{ 0.01 }; //original value is 0.01 but testing calculations suggest to use 0.1 TEMPORARY HACK FIX USE 10.0 FOR EXTRA VISCOUS
	//this is a minimum "safety-net" for SPH which is quite sensitive to the parameters.
	double m_pseudoViscossityCoefficient{ 10.0 }; //this used to be 10.0
	//Speed of sound in the medium to determine the stiffness of the system.
	//This should be the actual speed of sound in the fluid but a lower value is better to trace-off performance and compressibility
	double m_speedOfSound{ 100.0 };

	//these are needed for post processing
	TArray<FVector> m_newPositions;
	TArray<FVector> m_newVelocities;

	//rigid body obstacle in the simulation
	TArray<class ACollider*> m_colliders;
	//Where the particles spawn from (like a fountain) THIS IS FOR FUTURE IMPLEMENTATION
	//class AParticleEmitter* m_emitter;

public:	
	// Sets default values for this component's properties
	AParticleSystemSolver();
	~AParticleSystemSolver() = default;

	void initPhysicsSolver(TArray<class AFluidParticle*>* ptrParticles, class AFluidSimulation_FYPGameModeBase* gameMode);
	void OnAdvanceTimeStep(double timeIntervalInSeconds);

	//Vector fields include wind, water current... even colours
	const FVector SampleVectorField(const FVector& _subject, const FVector& _vectorField) const;
	//Scalar fields include temperature, pressure
	const double SampleScalarField(const FVector& _subject) const;

protected:
	TArray<class AFluidParticle*>* m_ptrParticles;
	class AFluidSimulation_FYPGameModeBase* m_gameMode;
	//zero means clamping, one means do nothing
	double m_negaitvePressureScale{ 0.0 };

	virtual void onBeginAdvanceTimeStep();
	virtual void accumulateForces(double timeStepInSeconds);
	void accumulateExternalForces(double timeStepInSeconds);
	void accumulateNonPressureForces(double timeStepInSeconds);
	virtual void accumulatePressureForce(double timeStepInSeconds);
	void accumulateViscosityForce();
	void computePressure();
	void computePseudoViscosity(double timeStepInSeconds);
	double computePressureFromEOS(double density, double targetDensity, double eosScale, double eosExponent, double negativePressureScale);

	//only external forces will be taken into account here.
	void resolveCollision(TArray<FVector>* positions, TArray<FVector>* velocities);		
};