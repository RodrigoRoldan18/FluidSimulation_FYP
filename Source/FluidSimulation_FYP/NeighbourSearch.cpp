// Fill out your copyright notice in the Description page of Project Settings.


#include "NeighbourSearch.h"
#include "FluidParticle.h"

size_t UNeighbourSearch::getHashKeyFromPosition(const FVector& pos)
{
	return 0;
}

// Sets default values for this component's properties
UNeighbourSearch::UNeighbourSearch()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UNeighbourSearch::build(const TArray<class AFluidParticle*>& points)
{
	m_buckets.Empty();
	m_particlePositions.Empty();

	if (points.Num() == 0)
	{
		return;
	}

	//Allocate memory chuncks
	m_buckets.Reserve(m_resolution.X * m_resolution.Y * m_resolution.Z);
	m_particlePositions.Reserve(points.Num());

	//Put points into buckets
	for (size_t i = 0; i < points.Num(); i++)
	{
		m_particlePositions[i] = points[i]->GetParticlePosition();
		size_t key = getHashKeyFromPosition(m_particlePositions[i]);
		m_buckets[key].Push(i);
	}
}

// Called when the game starts
void UNeighbourSearch::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}