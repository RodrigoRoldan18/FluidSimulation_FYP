// Fill out your copyright notice in the Description page of Project Settings.


#include "Collider.h"

// Sets default values
ACollider::ACollider()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	m_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = m_mesh;
}

void ACollider::ResolveCollision(const FVector& currentPosition, const FVector& currentVelocity, double radius, double restitutionCoefficient, FVector* newPosition, FVector* newVelocity)
{
	ColliderQueryResult colliderPoint;

	GetClosestPoint(m_surface, *newPosition, &colliderPoint);

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

void ACollider::GetClosestPoint(USurface* surface, const FVector& queryPoint, ColliderQueryResult* result) const
{
	result->distance = surface->ClosestDistance(queryPoint); //Get closest distance from querypoint to the mesh
	result->point = surface->ClosestPoint(queryPoint); //get the closest point on the mesh to the querypoint
	result->normal = surface->ClosestNormal(queryPoint); //get the normal
	result->velocity = VelocityAt(queryPoint); //

	//I AM OVERCOMPLICATING MYSELF. TAKE A STEP BACK AND RECONSIDER THE APPROACH.
	//might not need to create a surface class. The mesh should be enough to help.
}

bool ACollider::IsPenetrating(const ColliderQueryResult& colliderPoint, const FVector& position, double radius)
{
	//If the new candidate position of the particle is on the other side of the surface OR the new distance to the
	//surface is less than the particle's radius, this particle is in colliding state.
	return FVector::DotProduct((position - colliderPoint.point), colliderPoint.normal) < 0.0f || colliderPoint.distance < radius;
}

USurface::USurface()
{
	PrimaryComponentTick.bCanEverTick = false;
	m_transform = FTransform();
}

double USurface::ClosestDistance(const FVector& otherPoint) const
{
	return FVector::Distance(otherPoint, ClosestPoint(otherPoint)); //otherPoint should become to transform.toLocal(otherPoint)

	//toLocal function is supposed to: inverseOrientationMatrix * (otherPoint * _translation)
}

FVector USurface::ClosestNormal(const FVector& otherPoint) const
{
	FVector result; // = m_transform.ToWorldDirection(m_normal);
	//toWorldDirection function is supposed to: orientationMatrix * m_normal
	//if the normal is flipped just multiply it by -1
	return result;
}

FVector USurface::ClosestPoint(const FVector& otherPoint) const
{
	//closest point local
	FVector r = otherPoint - m_point; //otherPoint should become to transform.toLocal(otherPoint)
	FVector closestPointLocalResult = r - FVector::DotProduct(m_normal, r) * m_normal + m_point;

	FVector result; // = m_transform.toWorld(closestPointLocalResult);
	//toWorld function is supposed to: orientationMatrix * pointInLocal + _translation
	return result;
}
