// Fill out your copyright notice in the Description page of Project Settings.


#include "PointParticleEmitter.h"
#include "FluidParticle.h"
#include "Components/ArrowComponent.h"

// Sets default values
APointParticleEmitter::APointParticleEmitter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	m_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = m_mesh;

	m_arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
}

void APointParticleEmitter::Initialise(TArray<class AFluidParticle*>* ptrParticles)
{
	m_ptrParticles = ptrParticles;

	//Setup timer event
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Apparently the world doesn't exist"));
		return;
	}

	float timeInterval = 1.0f / m_maxNumOfParticlesPerSecond;
	UE_LOG(LogTemp, Warning, TEXT("the time interval for the timed event is: %f"), timeInterval);
	World->GetTimerManager().SetTimer(loopTimeHandle, this, &APointParticleEmitter::Emit, timeInterval, true);
}

void APointParticleEmitter::Emit()
{
	if (m_ptrParticles == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Array of particles didn't load properly in the emitter"));
		GetWorldTimerManager().ClearTimer(loopTimeHandle);
		return;
	}
	if (m_numOfEmittedParticles >= m_maxNumOfParticles)
	{
		UE_LOG(LogTemp, Warning, TEXT("Teapot will stop spawning particles"));
		GetWorldTimerManager().ClearTimer(loopTimeHandle);
		return;
	}

	//Spawn a particle
	FCriticalSection Mutex;
	FVector newParticleLocation = m_arrow->GetComponentLocation();
	AFluidParticle* newParticle = GetWorld()->SpawnActor<AFluidParticle>(ParticleBP, newParticleLocation, FRotator().ZeroRotator);
	FVector newParticleVelocity = m_speed * (FMath::VRandCone(m_arrow->GetForwardVector(), (m_spreadAngleInDegrees * 3.14 / 180.0)));
	Mutex.Lock();
	newParticle->SetParticlePosition(newParticleLocation);
	newParticle->SetParticleVelocity(newParticleVelocity);
	m_ptrParticles->Push(newParticle);
	Mutex.Unlock();
	m_numOfEmittedParticles++;
}

FVector APointParticleEmitter::UniformSampleCone(double rand1, double rand2, const FVector& axis, double angle)
{
	double cosAngle_2 = FMath::Cos(angle / 2.0);
	double z = 1 - (1 - cosAngle_2) * rand1;
	double r = FMath::Sqrt(FMath::Max<float>(0, 1 - z * z));
	double phi = 3.14 * 2.0 * rand2;
	double x = r * FMath::Cos(phi);
	double y = r * FMath::Sin(phi);
	
	//tangential vector
	FVector a = ((FMath::Abs<float>(z) > 0 || FMath::Abs<float>(y) > 0) ? FVector(1, 0, 0) : FVector(0, 0, 1));
	FVector tangential_1 = FVector::CrossProduct(a, axis).GetSafeNormal();
	FVector tangential_2 = FVector::CrossProduct(axis, a);
	FVector result = tangential_1 * x + axis * z + tangential_2 * y;
	return result;
}
