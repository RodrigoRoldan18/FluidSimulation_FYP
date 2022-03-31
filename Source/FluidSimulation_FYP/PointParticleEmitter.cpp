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
	FVector newParticleVelocity = m_speed * m_arrow->GetForwardVector();
	Mutex.Lock();
	newParticle->SetParticlePosition(newParticleLocation);
	newParticle->SetParticleVelocity(newParticleVelocity);
	m_ptrParticles->Push(newParticle);
	Mutex.Unlock();
	m_numOfEmittedParticles++;
}