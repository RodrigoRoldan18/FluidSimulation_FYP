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
	FVector m_velocity{ FVector(0.0f, 0.0f, 10.0f) };
	FVector m_force{ 0.0f };
	double m_density;
	double m_pressure;
	
public:	
	static const double kRadius;
	static const double kMass;

	// Sets default values for this actor's properties
	AFluidParticle();

	//Setters
	void SetParticlePosition(const FVector& _position) { m_position = _position; }
	void SetParticleVelocity(const FVector& _velocity) { m_velocity = _velocity; }
	void SetParticleForce(const FVector& _force) { m_force = _force; }
	void SetParticleDensity(double _density) { m_density = _density; }
	void SetParticlePressure(double _pressure) { m_pressure = _pressure; }

	//Getters
	const FVector& GetParticlePosition() const { return m_position; }
	const FVector& GetParticleVelocity() const { return m_velocity; }
	const FVector& GetParticleForce() const { return m_force; }
	double GetParticleDensity() const { return m_density; }
	double GetParticlePressure() const { return m_pressure; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
