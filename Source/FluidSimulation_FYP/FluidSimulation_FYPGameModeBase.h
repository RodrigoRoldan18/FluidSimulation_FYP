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

	void BuildNeighbourSearcher(float maxSearchRadius);
	void BuildNeighbourLists(float maxSearchRadius);

	size_t GetNumberOfParticles() const { return m_particles.Num(); }
	TArray<class AFluidParticle*>* GetParticleArrayPtr() { return &m_particles; }

	//Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
};

USTRUCT()
struct FSphStdKernel
{
	GENERATED_BODY()

	double h;

	FSphStdKernel();
	explicit FSphStdKernel(double kernelRadius);
	FSphStdKernel(const FSphStdKernel& other);
	double operator()(double distance) const;
};

inline FSphStdKernel::FSphStdKernel() : h(0) {}
inline FSphStdKernel::FSphStdKernel(double kernelRadius) : h(kernelRadius) {}
inline FSphStdKernel::FSphStdKernel(const FSphStdKernel& other) : h(other.h) {}
inline double FSphStdKernel::operator()(double distance) const
{
	if (distance * distance >= h * h)
	{
		return 0.0f;
	}
	else
	{
		double x = 1.0f - distance * distance / (h * h);
		return 315.0f / (64.0f * 3.14f * (h * h * h)) * x * x * x;
	}
}