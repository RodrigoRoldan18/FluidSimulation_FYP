// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"

AFluidSimulation_FYPGameModeBase::AFluidSimulation_FYPGameModeBase()
{
}

AFluidSimulation_FYPGameModeBase::AFluidSimulation_FYPGameModeBase(size_t numberOfParticles, const FVector2D& _simDimensions)
	: m_simulationDimensions(_simDimensions)
{
	static FVector newParticleLocation;
	const size_t particleLimitX = m_simulationDimensions.X / kParticleRadius;
	const size_t particleLimitY = m_simulationDimensions.Y / kParticleRadius;

	resize(numberOfParticles);

	for (size_t i = 0; i < numberOfParticles; i++)
	{
		newParticleLocation = FVector(kParticleRadius * i, kParticleRadius * i, 0.0f);
		AFluidParticle* newParticle = GetWorld()->SpawnActor<AFluidParticle>(newParticleLocation, FRotator().ZeroRotator);
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
