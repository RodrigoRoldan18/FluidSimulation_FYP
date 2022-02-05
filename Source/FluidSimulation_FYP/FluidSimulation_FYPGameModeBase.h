// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/Public/HAL/Runnable.h"
#include "Core/Public/HAL/RunnableThread.h"
#include "FluidSimulation_FYPGameModeBase.generated.h"

/**
 FOR NOW I WILL USE THE GAME MODE AS THE PARTICLE SYSTEM DATA MANAGER 

 Meyers Singleton (thread safe)
 */
UCLASS()
class FLUIDSIMULATION_FYP_API AFluidSimulation_FYPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
private:	
	TArray<class AFluidParticle*> m_particles;
	const float kParticleRadius{ 10.0f }; // default size of UE4 sphere model.
	int m_numOfParticles{ 1000 };
	FVector2D m_simulationDimensions { FVector2D(100.0f) };

	UPROPERTY(EditDefaultsOnly, Category = "Particle")
	TSubclassOf<class AFluidParticle> ParticleBP;

public:
	AFluidSimulation_FYPGameModeBase() = default;
	~AFluidSimulation_FYPGameModeBase() = default;

	void initSimulation();
	void resize(size_t newNumberOfParticles);

	size_t GetNumberOfParticles() const { return m_particles.Num(); }
	const class AFluidParticle* const particles() const;
	void AddParticles(const TArray<class AFluidParticle>& newParticles);

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

	void PrintCalcData();
	int32 ProcessedCalculation();

	class FThreadCalculator* CalcThread = nullptr;
	FRunnableThread* CurrentRunningThread = nullptr;
};
//------------------------------------------------------------------------------------
//Forward declarations
class FRunnableThread;
class AFluidParticle;

class AThreadCalculator : public FRunnable
{
public:
	// Sets default values for this actor's properties
	AThreadCalculator(int32 _numOfParticles);

	bool bStopThread;

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

private:
	int32 Calculations;
	int32 CalcCount;

};

//------------------------------------------------------------------------------------
class ParticleSpawnTask : public FNonAbandonableTask
{
public:
	ParticleSpawnTask(int32 _numOfParticles);

	~ParticleSpawnTask();

	//required by UE4
	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(ParticleSpawnTask, STATGROUP_ThreadPoolAsyncTasks); }

	int32 m_numOfParticles;

	void DoWork();
};