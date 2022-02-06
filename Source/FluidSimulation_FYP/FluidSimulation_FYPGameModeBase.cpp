// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"

//The default initialisation will have 100 particles in an area of 40 by 40 from the origin.
void AFluidSimulation_FYPGameModeBase::initSimulation()
{
	resize(m_numOfParticles);

	//thread disabled for now
	//CalcThread = new FThreadCalculator(m_numOfParticles, this);
	//CurrentRunningThread = FRunnableThread::Create(CalcThread, TEXT("Calculation Thread"));

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
		m_particles.Push(newParticle);
	}
}

void AFluidSimulation_FYPGameModeBase::resize(size_t newNumberOfParticles)
{
	m_particles.Reserve(newNumberOfParticles);
}

void AFluidSimulation_FYPGameModeBase::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentRunningThread && CalcThread)
	{
		CurrentRunningThread->Suspend(true);
		CalcThread->bStopThread = true;
		CurrentRunningThread->Suspend(false);
		CurrentRunningThread->Kill(false);
		CurrentRunningThread->WaitForCompletion();
		delete CalcThread;
	}
}

void AFluidSimulation_FYPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	initSimulation();
}

//---------------------------------------------------------------------------
FThreadCalculator::FThreadCalculator(int32 _numOfParticles, AFluidSimulation_FYPGameModeBase* _gameMode)
{
	if (_numOfParticles > 0 && _gameMode)
	{
		NumOfParticles = _numOfParticles;
		CurrentGameMode = _gameMode;
	}
}

bool FThreadCalculator::Init()
{
	bStopThread = false;
	ParticlesSpawned = 0;

	return true;
}

uint32 FThreadCalculator::Run()
{
	while (!bStopThread)
	{
		if (ParticlesSpawned < NumOfParticles)
		{
			//spawn the particle here
			ParticlesSpawned++;
		}
		else
		{
			bStopThread = true;
		}
	}
	return 0;
}

void FThreadCalculator::Stop()
{
}