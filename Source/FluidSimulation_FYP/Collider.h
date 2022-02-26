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

protected:
	struct ColliderQueryResult {
		double distance;
		FVector point;
		FVector normal;
		FVector velocity;
	};

	void GetClosestPoint(class USurface* surface, const FVector& queryPoint, ColliderQueryResult* result) const;
	bool IsPenetrating(const ColliderQueryResult& colliderPoint, const FVector& position, double radius);
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLUIDSIMULATION_FYP_API USurface : public UActorComponent
{
	GENERATED_BODY()

public:
	//local-to-world transform
	FTransform m_transform;
	FVector m_normal = FVector(0, 0, 1);
	//point that lies on the plane
	FVector m_point;

	USurface(const FTransform& transform) : m_transform(transform) {}
	USurface();

	//closest distance from the given point to the point on the surface
	double ClosestDistance(const FVector& otherPoint) const;
	//normal to the closest point on the surface from the given point
	FVector ClosestNormal(const FVector& otherPoint) const;
	//closest point from the given point to the surface
	FVector ClosestPoint(const FVector& otherPoint) const;
};