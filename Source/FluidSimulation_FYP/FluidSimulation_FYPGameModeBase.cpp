// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "NeighbourSearch.h"
#include "ParticleSystemSolver.h"

AFluidSimulation_FYPGameModeBase::AFluidSimulation_FYPGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

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