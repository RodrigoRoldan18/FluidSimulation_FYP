// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"

//The default initialisation will have 100 particles in an area of 40 by 40 from the origin.
void AFluidSimulation_FYPGameModeBase::initSimulation()
{
	resize(m_numOfParticles);

	//(new FAutoDeleteAsyncTask<ParticleSpawnTask>())->StartBackgroundTask();
	//CalcThread = new FThreadCalculator(m_numOfParticles);
	//CurrentRunningThread = FRunnableThread::Create(CalcThread, TEXT("Calculation Thread"));

	static FVector newParticleLocation;
	int particleLimitX = m_simulationDimensions.X / kParticleRadius;
	int numColumn = 0;
	int numLevel = 1;
	int indexToResetPosition = 0;

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

		AFluidParticle* newParticle = GetWorld()->SpawnActor<AFluidParticle>(ParticleBP, newParticleLocation, FRotator().ZeroRotator);
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

void AFluidSimulation_FYPGameModeBase::EndPlay(EEndPlayReason::Type EndPlayReason)
{
}

void AFluidSimulation_FYPGameModeBase::BeginPlay()
{
	initSimulation();
}

void AFluidSimulation_FYPGameModeBase::PrintCalcData()
{
	if (!m_particles.Num())
	{

	}
}

int32 AFluidSimulation_FYPGameModeBase::ProcessedCalculation()
{
	return int32();
}

//---------------------------------------------------------------------------
AThreadCalculator::AThreadCalculator(int32 _numOfParticles)
{
}

bool AThreadCalculator::Init()
{
	return false;
}

uint32 AThreadCalculator::Run()
{
	return uint32();
}

void AThreadCalculator::Stop()
{
}

//---------------------------------------------------------------------------

ParticleSpawnTask::ParticleSpawnTask(int32 _numOfParticles)
	: m_numOfParticles(_numOfParticles)
{
}

ParticleSpawnTask::~ParticleSpawnTask()
{
	UE_LOG(LogTemp, Warning, TEXT("Task Finished!!"));
}

void ParticleSpawnTask::DoWork()
{
	
}