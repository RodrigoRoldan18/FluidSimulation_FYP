// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/Public/HAL/Runnable.h"
#include "Core/Public/HAL/RunnableThread.h"
#include "FluidSimulation_FYPGameModeBase.generated.h"

/**
 FOR NOW I WILL USE THE GAME MODE AS THE PARTICLE SYSTEM DATA MANAGER 
 */
UCLASS()
class FLUIDSIMULATION_FYP_API AFluidSimulation_FYPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	friend class FThreadCalculator;
	
private:	
	TArray<class AFluidParticle*> m_particles;
	const float kParticleRadius{ 10.0f }; // default size of UE4 sphere model is 100 but I scaled it down.
	const FIntVector kDefaultHashGridResolution{ FIntVector(10) };
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
	float GetParticleRadius() const { return kParticleRadius; }

	//Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

	class FThreadCalculator* CalcThread = nullptr;
	FRunnableThread* CurrentRunningThread = nullptr;
};


//------------------------------------------------------------------------------------
//Forward declarations
class FRunnableThread;
class AFluidParticle;

class FThreadCalculator : public FRunnable
{
public:
	// Sets default values for this actor's properties
	FThreadCalculator(int32 _numOfParticles, AFluidSimulation_FYPGameModeBase* _gameMode);

	bool bStopThread;

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

private:
	int32 NumOfParticles;
	int32 ParticlesSpawned;
	AFluidSimulation_FYPGameModeBase* CurrentGameMode;
	AFluidParticle* CurrentParticle;

};