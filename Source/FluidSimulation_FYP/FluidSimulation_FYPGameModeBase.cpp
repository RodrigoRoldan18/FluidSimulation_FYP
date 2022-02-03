// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"

//The default initialisation will have 1000 particles in an area of 500 by 500 from the origin.
void AFluidSimulation_FYPGameModeBase::initSimulation()
{
	static FVector newParticleLocation;
	int particleLimitX = m_simulationDimensions.X / kParticleRadius;
	int numColumn = 0;
	int numLevel = 1;
	int indexToResetPosition = 0;

	resize(m_numOfParticles);

	for (int i = 0; i < m_numOfParticles; i++)
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

		AFluidParticle* newParticle = GetWorld()->SpawnActor<AFluidParticle>(ParticleBP ,newParticleLocation, FRotator().ZeroRotator);
		m_particles.Push(newParticle);
	}
}

void AFluidSimulation_FYPGameModeBase::resize(size_t newNumberOfParticles)
{
	m_particles.Reserve(newNumberOfParticles);
}

const AFluidParticle* const AFluidSimulation_FYPGameModeBase::particles() const
{
	return nullptr;
}

void AFluidSimulation_FYPGameModeBase::AddParticles(const TArray<class AFluidParticle>& newParticles)
{
}

void AFluidSimulation_FYPGameModeBase::BeginPlay()
{
	initSimulation();
}
