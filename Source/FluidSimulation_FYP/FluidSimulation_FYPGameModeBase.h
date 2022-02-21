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
	double m_targetDensity;
	//target spacing in meters
	double m_targetSpacing{ 100.0f }; //this should be 0.1f
	//kernel radius in meters
	double m_kernelRadius;
	double m_kernelRadiusOverTargetSpacing{ 1800.0f }; //this should be 1.8f

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

	FVector Interpolate(const FVector& origin, const TArray<FVector>& values) const;
	void UpdateDensities();
	double sumOfKernelNearby(const FVector& origin) const;
	FVector GradientAt(size_t i, const TArray<double>& values) const;
	double LaplacianAt(size_t i, const TArray<double>& values) const;

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

	//kernel radius
	double h;
	double h2;
	double h3;
	double h5;

	FSphStdKernel();
	explicit FSphStdKernel(double kernelRadius);
	FSphStdKernel(const FSphStdKernel& other);
	double operator()(double distance) const;
	double FirstDerivative(double distance) const;
	double SecondDerivative(double distance) const;
	FVector Gradient(double distance, const FVector& directionToCentre) const;
};

inline FSphStdKernel::FSphStdKernel() : h(0), h2(0), h3(0), h5(0) {}
inline FSphStdKernel::FSphStdKernel(double kernelRadius) : h(kernelRadius), h2(h * h), h3(h2 * h), h5(h2 * h3) {}
inline FSphStdKernel::FSphStdKernel(const FSphStdKernel& other) : h(other.h), h2(other.h2), h3(other.h3), h5(other.h5) {}
inline double FSphStdKernel::operator()(double distance) const
{
	//distance between particles (r)
	if (distance * distance >= h * h)
	{
		return 0.0f;
	}
	else
	{
		double x = 1.0f - distance * distance / h2;
		return 315.0f / (64.0f * 3.14f * h3) * x * x * x;
	}
}

inline double FSphStdKernel::FirstDerivative(double distance) const
{
	if (distance >= h)
	{
		return 0.0f;
	}
	else
	{
		double x = 1.0f - distance * distance / h2;
		return -945.0f / (32.0f * 3.14f * h5) * distance * x * x;
	}
}

inline double FSphStdKernel::SecondDerivative(double distance) const
{
	if (distance * distance >= h2)
	{
		return 0.0f;
	}
	else
	{
		double x = distance * distance / h2;
		return 945.0f / (32.0f * 3.14f * h5) * (1 - x) * (3 * x - 1);
	}
}

inline FVector FSphStdKernel::Gradient(double distance, const FVector& directionToCentre) const
{
	return -FirstDerivative(distance) * directionToCentre;
}