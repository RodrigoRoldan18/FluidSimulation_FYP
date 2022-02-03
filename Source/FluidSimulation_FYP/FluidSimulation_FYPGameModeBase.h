// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FluidSimulation_FYPGameModeBase.generated.h"

/**
 FOR NOW I WILL USE THE GAME MODE AS THE PARTICLE SYSTEM DATA MANAGER 
 */
UCLASS()
class FLUIDSIMULATION_FYP_API AFluidSimulation_FYPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
	TArray<class AFluidParticle*> m_particles;
	const float kParticleRadius{ 32.0f };
	int m_numOfParticles{ 1000 };
	FVector2D m_simulationDimensions { FVector2D(500.0f) };

	UPROPERTY(EditDefaultsOnly, Category = "Particle")
	TSubclassOf<class AFluidParticle> ParticleBP;

public:
	AFluidSimulation_FYPGameModeBase() = default;
	void initSimulation();
	void resize(size_t newNumberOfParticles);

	size_t GetNumberOfParticles() const { return m_particles.Num(); }

	const class AFluidParticle* const particles() const;

	void AddParticles(const TArray<class AFluidParticle>& newParticles);	

protected:
	virtual void BeginPlay() override;
};
