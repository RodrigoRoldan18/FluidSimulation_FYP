// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PointParticleEmitter.generated.h"

UCLASS()
class FLUIDSIMULATION_FYP_API APointParticleEmitter : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* m_mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UArrowComponent* m_arrow;

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	TSubclassOf<class AFluidParticle> ParticleBP;

	bool m_isEnabled{ true };
	TArray<class AFluidParticle*>* m_ptrParticles;

	FTimerHandle loopTimeHandle;

	size_t m_numOfEmittedParticles{ 0 };
	size_t m_maxNumOfParticlesPerSecond{ 10 };
	size_t m_maxNumOfParticles{ 200 };

	double m_speed{ 10.0 };
	double m_spreadAngleInDegrees{ 90.0 };

public:	
	// Sets default values for this actor's properties
	APointParticleEmitter();
	~APointParticleEmitter() = default;

	void Initialise(TArray<class AFluidParticle*>* ptrParticles);
	void Emit();
};
