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
	float m_mass{ 1.0f };
	
public:	
	// Sets default values for this actor's properties
	AFluidParticle();

	//Setters
	void SetParticlePosition(const FVector& _position) { m_position = _position; }
	void SetParticleVelocity(const FVector& _velocity) { m_velocity = _velocity; }
	void SetParticleForce(const FVector& _force) { m_force = _force; }
	void SetParticleMass(float _mass) { m_mass = _mass; }

	//Getters
	const FVector& GetParticlePosition() const { return m_position; }
	const FVector& GetParticleVelocity() const { return m_velocity; }
	const FVector& GetParticleForce() const { return m_force; }
	float GetParticleMass() const { return m_mass; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
