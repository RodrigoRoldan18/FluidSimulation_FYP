// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidParticle.generated.h"

UCLASS()
class FLUIDSIMULATION_FYP_API AFluidParticle : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	FVector m_position;
	FVector m_velocity;
	FVector m_force;
	
public:	
	// Sets default values for this actor's properties
	AFluidParticle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
