// Copyright Epic Games, Inc. All Rights Reserved.


#include "FluidSimulation_FYPGameModeBase.h"
#include "FluidParticle.h"
#include "NeighbourSearch.h"
#include "ParticleSystemSolver.h"
#include "PCISPH_Solver.h"
#include "Kernels.h"
#include "Async/Async.h"

AFluidSimulation_FYPGameModeBase::AFluidSimulation_FYPGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

	m_kernelRadius = m_kernelRadiusOverTargetSpacing * m_targetSpacing;
	UE_LOG(LogTemp, Warning, TEXT("Kernel radius: %f"), m_kernelRadius);

	m_neighbourSearcher = CreateDefaultSubobject<UNeighbourSearch>("NeighbourSearcher");
	if (m_usePCISPHsolver)
	{
		m_physicsSolver = CreateDefaultSubobject<APCISPH_Solver>("PhysicsSolver");
	}
	else
	{
		m_physicsSolver = CreateDefaultSubobject<AParticleSystemSolver>("PhysicsSolver");
	}
}

//The default initialisation will have 1000 particles in an area of 100 by 100 from the origin.
void AFluidSimulation_FYPGameModeBase::initSimulation()
{
	resize(GetNumberOfParticles());

	//return;

	static FVector newParticleLocation;
	int particleLimitX = m_simulationDimensions.X / kParticleRadius;
	int numColumn = 0;
	int numLevel = 1;
	int indexToResetPosition = 0;

	for (size_t i = 0; i < 4000; i++)
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
		//10.0f on X and Y moves the particles more to the middle, 2.0f on Z is to make them float a bit
		newParticleLocation = FVector(kParticleRadius * (i - indexToResetPosition - (particleLimitX * numColumn)) + 10.0f, kParticleRadius * numColumn + 10.0f, kParticleRadius * numLevel + 4.0f);

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

void AFluidSimulation_FYPGameModeBase::BuildNeighbourSearcher()
{
	m_neighbourSearcher->initialiseNeighbourSearcher(kDefaultHashGridResolution, m_kernelRadius); //I used to have 2 * kParticleRadius
	m_neighbourSearcher->build(m_particles);
}

void AFluidSimulation_FYPGameModeBase::BuildNeighbourLists()
{
	m_neighbourLists.Reserve(GetNumberOfParticles());
	m_neighbourLists.SetNumZeroed(GetNumberOfParticles());

	auto particles = m_particles;
	size_t n = particles.Num();
	ParallelFor(n, [&](size_t i) {
		FVector origin = particles[i]->GetParticlePosition();
		m_neighbourLists[i].Empty();

		m_neighbourSearcher->forEachNearbyPoint(origin, m_kernelRadius, [&](size_t j, const FVector&) { //kParticleRadius should be m_kernelRadius instead
			if (i != j)
			{
				m_neighbourLists[i].Add(j);
				/*if (i == 723)
				{
					UE_LOG(LogTemp, Warning, TEXT("Particle %i has this particle as neighbour: %i"), i, j);
				}*/
			}
			});
		/*if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle %i has %i particle neighbours"), i, m_neighbourLists[i].Num());
		}*/
		});
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

double AFluidSimulation_FYPGameModeBase::Interpolate(const FVector& origin, const TArray<double>& values) const
{
	double sum = 0.0;
	FSphStdKernel kernel(m_kernelRadius);
	const double mass = m_particles[0]->kMass;

	m_neighbourSearcher->forEachNearbyPoint(origin, m_kernelRadius, [&](size_t i, const FVector& neighbourPos) {
		double dist = FVector::Distance(origin, neighbourPos);
		//more weight the closer to the origin.
		double weight = mass * kernel(dist);
		sum += weight;
		});

	return sum;
}

//helper function to remove the infinite recursion from the function above.
//this function needs to be called first to initialise the densities
void AFluidSimulation_FYPGameModeBase::UpdateDensities()
{
	//Async(EAsyncExecution::Thread, [&]() {
	size_t n = m_particles.Num();
	FCriticalSection Mutex;
	ParallelFor(n, [&](size_t i) {
	//for (size_t i = 0; i < n; i++)		
		double sum = sumOfKernelNearby(m_particles[i]->GetParticlePosition(), i == 723 ? true : false);
		Mutex.Lock();
		m_particles[i]->SetParticleDensity(m_particles[i]->kMass * sum);
		Mutex.Unlock();
		/*if (i == 723)
		{
			UE_LOG(LogTemp, Warning, TEXT("Particle 723 DENSITY: %f. The sumOfKernelNearby is: %f"), m_particles[i]->GetParticleDensity(), sum);
		}*/
		
	});
}

double AFluidSimulation_FYPGameModeBase::sumOfKernelNearby(const FVector& origin, bool testPrint) const
{
	double sum = 0.0;
	FSphStdKernel kernel(m_kernelRadius);
	m_neighbourSearcher->forEachNearbyPoint(origin, m_kernelRadius, [&](size_t, const FVector& neighbourPos) {
		double dist = FVector::Distance(origin, neighbourPos);		
		sum += kernel(dist);
		if (testPrint)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Particle 723 distance: %f"), dist);
			//UE_LOG(LogTemp, Warning, TEXT("Particle 723 KERNEL distance: %f"), kernel(dist));
			//SumOfKernel calculations look fine
		}
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
		if (dist > 0.0)
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
	double sum = 0.0;
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

	//STAGE 1 - MEASURE DENSITY WITH PARTICLES' CURRENT LOCATIONS

	BuildNeighbourSearcher();
	BuildNeighbourLists();
	UpdateDensities();

	m_physicsSolver->OnAdvanceTimeStep(DeltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("DeltaTime: %f"), DeltaTime);

	//PrintCalcData();
}

void AFluidSimulation_FYPGameModeBase::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (m_currentRunningThread && m_baseThread)
	{
		//this simulates a mutex
		m_currentRunningThread->Suspend(true);
		m_baseThread->bStopThread = true;
		m_currentRunningThread->Suspend(false);
		m_currentRunningThread->Kill(false);
		m_currentRunningThread->WaitForCompletion();
		delete m_baseThread;
	}
}

void AFluidSimulation_FYPGameModeBase::InitThreadCalculations(int32 calculations)
{
	if (calculations > 0)
	{
		m_baseThread = new FBaseThread(calculations, this);
		m_currentRunningThread = FRunnableThread::Create(m_baseThread, TEXT("Base Thread"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Calculations must be greater than 0"));
	}
}

void AFluidSimulation_FYPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	initSimulation();
	m_physicsSolver->initPhysicsSolver(&m_particles, this);

	//InitThreadCalculations(50);
}

void AFluidSimulation_FYPGameModeBase::PrintCalcData()
{
	//Dequeue - removes and returns the item from the tail of the queue and holds it in m_processedCalculation
	if (!ThreadQueue.IsEmpty() && ThreadQueue.Dequeue(m_processedCalculation))
	{
		UE_LOG(LogTemp, Warning, TEXT("Precessed calculation: %d"), m_processedCalculation);
	}
}