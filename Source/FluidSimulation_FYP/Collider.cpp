// Fill out your copyright notice in the Description page of Project Settings.


#include "Collider.h"

// Sets default values
ACollider::ACollider()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	m_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = m_mesh;

	//m_location = m_mesh->GetComponentTransform().GetLocation();
}

void ACollider::ResolveCollision(double radius, double restitutionCoefficient, FVector* newPosition, FVector* newVelocity)
{
	ColliderQueryResult colliderPoint;

	GetQueryResult(*newPosition, &colliderPoint);

	//Check if the new position is penetrating the surface
	if (IsPenetrating(colliderPoint, *newPosition, radius))
	{
		//Target point is the closest non-penetrating position from the current position
		FVector targetNormal = colliderPoint.normal;
		FVector targetPoint = colliderPoint.point + radius * targetNormal;
		FVector colliderVelAtTargetPoint = colliderPoint.velocity;

		//get new candidate relative velocity from the target point.
		FVector relativeVel = *newVelocity - colliderVelAtTargetPoint;
		double normalDotRelativeVel = FVector::DotProduct(targetNormal, relativeVel);
		FVector relativeVelN = normalDotRelativeVel * targetNormal;
		FVector relativeVelT = relativeVel - relativeVelN;

		//Check if the velocity is facing opposite direction of the surface normal
		if (normalDotRelativeVel < 0.0)
		{
			//Apply restitution coefficient to the surface normal component of the velocity
			FVector deltaRelativeVelN = (-restitutionCoefficient - 1.0) * relativeVelN;
			relativeVelN *= -restitutionCoefficient;

			//Apply friction to the tangential component of the velocity
			if (relativeVelT.SizeSquared() > 0.0f)
			{
				double frictionScale = FMath::Max(1.0 - m_frictionCoefficient * deltaRelativeVelN.Size() / relativeVelT.Size(), 0.0);
				relativeVelT *= frictionScale;
			}

			//Reassemble the components
			*newVelocity = relativeVelN + relativeVelT + colliderVelAtTargetPoint;
		}

		//Geometry fix
		*newPosition = targetPoint;
	}
}

FVector ACollider::VelocityAt(const FVector& point) const
{
	FVector r = point - m_mesh->GetComponentTransform().GetTranslation(); //if we have to use the relative location we would use m_mesh->GetRelativeLocation();
	return m_linearVelocity + FVector::CrossProduct(m_angularVelocity, r);
}

double ACollider::ClosestDistance(const FVector& point) const
{
	//get closest distance to the surface
	return FVector::Distance(point, ClosestPoint(point));
}

FVector ACollider::ClosestPoint(const FVector& point) const
{
	//get closest point in the surface. This is causing some crashes. It might be because it's constantly refering to the mesh pointer.
	FVector r = point - m_location;
	FCriticalSection Mutex;
	Mutex.Lock();
	FVector closestPointResult = r - FVector::DotProduct(m_mesh->GetUpVector(), r) * m_mesh->GetUpVector() + m_mesh->GetComponentLocation();
	Mutex.Unlock();
	//UE_LOG(LogTemp, Warning, TEXT("Apparently, m_location is: %s"), *m_location.ToString());
	return closestPointResult;
}

void ACollider::GetQueryResult(const FVector& queryPoint, ColliderQueryResult* result)
{
	result->distance = ClosestDistance(queryPoint); //Get closest distance from querypoint to the mesh
	result->point = ClosestPoint(queryPoint); //get the closest point on the mesh to the querypoint
	result->normal = m_mesh->GetUpVector(); //get the normal
	//UE_LOG(LogTemp, Warning, TEXT("Apparently, the normal is: %s"), result.ToString());
	result->velocity = VelocityAt(queryPoint); 
}

bool ACollider::IsPenetrating(const ColliderQueryResult& colliderPoint, const FVector& position, double radius)
{
	//If the new candidate position of the particle is on the other side of the surface OR the new distance to the
	//surface is less than the particle's radius, this particle is in colliding state.
	return FVector::DotProduct((position - colliderPoint.point), colliderPoint.normal) < 0.0f || colliderPoint.distance < radius;
}

void ACollider::BeginPlay()
{
	Super::BeginPlay();

	m_location = m_mesh->GetComponentLocation();
}
