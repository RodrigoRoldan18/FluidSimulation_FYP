// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/Public/HAL/Runnable.h"
#include "Core/Public/HAL/RunnableThread.h"
#include "FluidSimulation_FYPGameModeBase.generated.h"

/**
 FOR NOW I WILL USE THE GAME MODE AS THE PARTICLE SYSTEM DATA MANAGER
 AND THE MAIN SIMULATION THREAD
 */
UCLASS()
class FLUIDSIMULATION_FYP_API AFluidSimulation_FYPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

private:
	const float kParticleRadius{ 10.0f }; // default size of UE4 sphere model is 100 but I scaled it down.
	const FIntVector kDefaultHashGridResolution{ FIntVector(10) };

	TArray<class AFluidParticle*> m_particles;
	class AParticleSystemSolver* m_physicsSolver;
	class UNeighbourSearch* m_neighbourSearcher;
	TArray<TArray<size_t>> m_neighbourLists;

	//water density in kg/m^3
	double m_targetDensity = 1000.0;
	//target spacing in meters
	double m_targetSpacing{ 100.0 }; //this should be 0.1f
	//kernel radius in meters
	double m_kernelRadius;
	double m_kernelRadiusOverTargetSpacing{ 1800.0 }; //this should be 1.8f

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	int32 m_numOfParticles{ 1000 };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	FVector2D m_simulationDimensions { FVector2D(100.0f) };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	TSubclassOf<class AFluidParticle> ParticleBP;

public:
	AFluidSimulation_FYPGameModeBase();
	~AFluidSimulation_FYPGameModeBase() = default;

	void initSimulation();
	void resize(size_t newNumberOfParticles);

	void BuildNeighbourSearcher();
	void BuildNeighbourLists();

	//Density computation
	FVector Interpolate(const FVector& origin, const TArray<FVector>& values) const;
	void UpdateDensities();
	double sumOfKernelNearby(const FVector& origin) const;
	FVector GradientAt(size_t i, const TArray<double>& values) const;
	double LaplacianAt(size_t i, const TArray<double>& values) const;

	size_t GetNumberOfParticles() const { return m_particles.Num(); }
	TArray<class AFluidParticle*>* GetParticleArrayPtr() { return &m_particles; }
	double GetTargetDensity() const { return m_targetDensity; }
	double GetKernelRadius() const { return m_kernelRadius; }
	TArray<TArray<size_t>>* GetNeighbourLists() { return &m_neighbourLists; }

	//Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
};