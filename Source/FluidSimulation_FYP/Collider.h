// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Collider.generated.h"

UCLASS()
class FLUIDSIMULATION_FYP_API ACollider : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* m_mesh;

	double m_frictionCoefficient{ 0.0 };
	FVector m_linearVelocity = FVector(0.0f, 0.0f, 0.0f);
	FVector m_angularVelocity = FVector(0.0f, 0.0f, 0.0f);
	
public:	
	// Sets default values for this actor's properties
	ACollider();

	void ResolveCollision(const FVector& currentPosition, const FVector& currentVelocity, double radius, double restitutionCoefficient, FVector* newPosition, FVector* newVelocity);
	FVector VelocityAt(const FVector& point) const;
	double ClosestDistance(const FVector& point) const;
	FVector ClosestPoint(const FVector& point) const;

protected:
	struct ColliderQueryResult {
		double distance;
		FVector point;
		FVector normal;
		FVector velocity;
	};

	void GetQueryResult(const FVector& queryPoint, ColliderQueryResult* result);
	bool IsPenetrating(const ColliderQueryResult& colliderPoint, const FVector& position, double radius);
};