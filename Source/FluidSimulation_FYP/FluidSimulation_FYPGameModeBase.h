// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaseThread.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
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
	const float kParticleRadius{ 1.0f }; // default size of UE4 sphere model is 100 but I scaled it down.
	const FIntVector kDefaultHashGridResolution{ FIntVector(64) }; //this was set to 10 before

	TArray<class AFluidParticle*> m_particles;
	class AParticleSystemSolver* m_physicsSolver;
	class UNeighbourSearch* m_neighbourSearcher;
	TArray<TArray<size_t>> m_neighbourLists;

	//water density in kg/m^3
	double m_targetDensity{ 1.0 }; //this should be 1000.0 but the pressure computation keeps returning negative values TEMPORARY HACK FIX IS 1.0
	//target spacing in meters
	double m_targetSpacing{ 1.0 }; //this should be 0.02 based on the water drop test but we are keeping it as 1 because of the particle radius 1.5 worked fine for 2000 particles. Particle radius is 1.0 therefore targetSpacing should be 20.0 (It's too big imo)
	//kernel radius in meters
	double m_kernelRadius;
	double m_kernelRadiusOverTargetSpacing{ 1.8 };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	int32 m_numOfParticles{ 5000 };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	bool m_usePCISPHsolver{ true };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	bool m_isFluidViscous{ true };

	UPROPERTY(EditDefaultsOnly, Category = "FluidSimulation")
	FVector2D m_simulationDimensions { FVector2D(60.0f, 10.0f) };

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
	//Returns interpolated vector data. Could be used for velocity and acceleration.
	FVector Interpolate(const FVector& origin, const TArray<FVector>& values) const;
	//Returns interpolated scalar data. Could be used for density and pressure.
	double Interpolate(const FVector& origin, const TArray<double>& values) const;
	void UpdateDensities();
	double sumOfKernelNearby(const FVector& origin, bool testPrint) const;
	FVector GradientAt(size_t i, const TArray<double>& values) const;
	double LaplacianAt(size_t i, const TArray<double>& values) const;

	size_t GetNumberOfParticles() const { return m_numOfParticles; }
	TArray<class AFluidParticle*>* GetParticleArrayPtr() { return &m_particles; }
	double GetTargetDensity() const { return m_targetDensity; }
	double GetKernelRadius() const { return m_kernelRadius; }
	double GetTargetSpacing() const { return m_targetSpacing; }
	bool IsUsingPCISPH() const { return m_usePCISPHsolver; }
	bool IsFluidViscous() const { return m_isFluidViscous; }
	TArray<TArray<size_t>>* GetNeighbourLists() { return &m_neighbourLists; }

	//Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "ThreadCalculations")
	void InitThreadCalculations(int32 calculations);

	TQueue<int32> ThreadQueue;

protected:
	virtual void BeginPlay() override;
	
	void PrintCalcData();
	int32 m_processedCalculation;

	class FBaseThread* m_baseThread = nullptr;

	//hold the container
	FRunnableThread* m_currentRunningThread = nullptr;
};