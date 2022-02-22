// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "NeighbourSearch.h"
#include "ParticleSystemSolver.h"
#include "Kernels.h"

AFluidSimulation_FYPGameModeBase::AFluidSimulation_FYPGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

	m_kernelRadius = m_kernelRadiusOverTargetSpacing * m_targetSpacing;

	m_neighbourSearcher = CreateDefaultSubobject<UNeighbourSearch>("NeighbourSearcher");
	m_physicsSolver = CreateDefaultSubobject<AParticleSystemSolver>("PhysicsSolver");
}

//The default initialisation will have 1000 particles in an area of 100 by 100 from the origin.
void AFluidSimulation_FYPGameModeBase::initSimulation()
{
	resize(m_numOfParticles);

	static FVector newParticleLocation;
	int particleLimitX = m_simulationDimensions.X / kParticleRadius;
	int numColumn = 0;
	int numLevel = 1;
	int indexToResetPosition = 0;

	for (size_t i = 0; i < m_numOfParticles; i++)
	{
		if (kParticleRadius * (i - indexToResetPosition - (particleLimitX * numColumn)) > m_simulationDimensions.X)
		{
			//move to new column
			numColumn++;
		}
		if (kParticleRadius * numColumn > m_simulationDimensions.Y)
		{
			//move to a new level
			numLevel++;
			numColumn = 0;
			indexToResetPosition = i - 1;
		}
		newParticleLocation = FVector(kParticleRadius * (i - indexToResetPosition - (particleLimitX * numColumn)), kParticleRadius * numColumn, kParticleRadius * numLevel);

		AFluidParticle* newParticle = GetWorld()->SpawnActor<AFluidParticle>(ParticleBP, newParticleLocation, FRotator().ZeroRotator);
		newParticle->SetParticlePosition(newParticleLocation);
		m_particles.Push(newParticle);
	}
	//UE_LOG(LogTemp, Warning, TEXT("particles in the array at the end: %i"), m_particles.Num());
}

void AFluidSimulation_FYPGameModeBase::resize(size_t newNumberOfParticles)
{
	m_particles.Reserve(newNumberOfParticles);
}

void AFluidSimulation_FYPGameModeBase::BuildNeighbourSearcher(float maxSearchRadius)
{
	m_neighbourSearcher->initialiseNeighbourSearcher(kDefaultHashGridResolution, 2.0f * maxSearchRadius);
	m_neighbourSearcher->build(m_particles);
}

void AFluidSimulation_FYPGameModeBase::BuildNeighbourLists(float maxSearchRadius)
{
	m_neighbourLists.Reserve(GetNumberOfParticles());
	m_neighbourLists.SetNumZeroed(GetNumberOfParticles());

	auto particles = m_particles;
	for (size_t i = 0; i < GetNumberOfParticles(); ++i)
	{
		FVector origin = particles[i]->GetParticlePosition();
		m_neighbourLists[i].Empty();

		m_neighbourSearcher->forEachNearbyPoint(origin, maxSearchRadius, [&](size_t j, const FVector&) {
			if (i != j)
			{
				m_neighbourLists[i].Add(j);
				if (i == 303)
				{
					UE_LOG(LogTemp, Warning, TEXT("Particle %i has this particle as neighbour: %i"), i, j);
				}				
			}
		});
	}
}

FVector AFluidSimulation_FYPGameModeBase::Interpolate(const FVector& origin, const TArray<FVector>& values) const
{
	FVector sum;
	FSphStdKernel kernel(m_kernelRadius);
	const double mass = m_particles[0]->kMass;

	m_neighbourSearcher->forEachNearbyPoint(origin, m_kernelRadius, [&](size_t i, const FVector& neighbourPos) {
		double dist = FVector::Distance(origin, neighbourPos);
		//more weight the closer to the origin.
		double weight = mass / m_particles[i]->GetParticleDensity() * kernel(dist);
		sum += weight * values[i];
		});

	return sum;
}

//helper function to remove the infinite recursion from the function above.
//this function needs to be called first to initialise the densities
void AFluidSimulation_FYPGameModeBase::UpdateDensities()
{
	size_t n = GetNumberOfParticles();
	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
		double sum = sumOfKernelNearby(m_particles[i]->GetParticlePosition());
		Mutex.Lock();
		m_particles[i]->SetParticleDensity(m_particles[i]->kMass * sum);
		Mutex.Unlock();
		});
}

double AFluidSimulation_FYPGameModeBase::sumOfKernelNearby(const FVector& origin) const
{
	double sum = 0.0f;
	FSphStdKernel kernel(m_kernelRadius);
	m_neighbourSearcher->forEachNearbyPoint(origin, m_kernelRadius, [&](size_t i, const FVector& neighbourPos) {
		double dist = FVector::Distance(origin, neighbourPos);
		sum += kernel(dist);
		});
	return sum;
}

//returns the symmetric gradient for the input values for a given particle with index i
FVector AFluidSimulation_FYPGameModeBase::GradientAt(size_t i, const TArray<double>& values) const
{
	FVector sum;
	const TArray<size_t>& neighbours = m_neighbourLists[i];
	FVector origin = m_particles[i]->GetParticlePosition();
	FSphSpikyKernel kernel(m_kernelRadius);

	for (size_t j : neighbours)
	{
		FVector neighbourPos = m_particles[j]->GetParticlePosition();
		double dist = FVector::Distance(origin, neighbourPos);
		if (dist > 0.0f)
		{
			FVector dir = (neighbourPos - origin) / dist;
			sum += m_particles[i]->GetParticleDensity() * m_particles[i]->kMass * 
				((values[i] / (m_particles[i]->GetParticleDensity() * m_particles[i]->GetParticleDensity())) +
				values[j] / (m_particles[j]->GetParticleDensity() * m_particles[j]->GetParticleDensity())) * kernel.Gradient(dist, dir);
		}
	}

	return sum;
}

double AFluidSimulation_FYPGameModeBase::LaplacianAt(size_t i, const TArray<double>& values) const
{
	double sum = 0.0f;
	const TArray<size_t>& neighbours = m_neighbourLists[i];
	FVector origin = m_particles[i]->GetParticlePosition();
	FSphSpikyKernel kernel(m_kernelRadius);

	for (size_t j : neighbours)
	{
		FVector neighbourPos = m_particles[j]->GetParticlePosition();
		double dist = FVector::Distance(origin, neighbourPos);
		sum += m_particles[j]->kMass * (values[j] - values[i]) / m_particles[j]->GetParticleDensity() * kernel.SecondDerivative(dist);		
	}

	return sum;
}

void AFluidSimulation_FYPGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	m_physicsSolver->OnAdvanceTimeStep(DeltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("TESTING GAMEMODE TICK"));
}

void AFluidSimulation_FYPGameModeBase::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AFluidSimulation_FYPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	initSimulation();
	m_physicsSolver->initPhysicsSolver(&m_particles);
	BuildNeighbourSearcher(kParticleRadius);
	BuildNeighbourLists(kParticleRadius);
}