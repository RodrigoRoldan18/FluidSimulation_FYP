// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeighbourSearch.generated.h"

//grid based hashing algorithm
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLUIDSIMULATION_FYP_API UNeighbourSearch : public UActorComponent
{
	GENERATED_BODY()

	double m_gridSpacing = 1.0f;
	FIntVector m_resolution = FIntVector(1, 1, 1);
	TArray<TArray<size_t>> m_buckets;
	TArray<FVector> m_particlePositions;

	size_t getHashKeyFromPosition(const FVector& pos) const;
	FIntVector getBucketIndex(const FVector& pos) const;
	size_t getHashKeyFromBucketIndex(const FIntVector& bucketIndex) const;
	void getNearbyKeys(const FVector& pos, size_t* nearbyKeys) const;

public:	
	// Sets default values for this component's properties
	UNeighbourSearch();

	void build(const TArray<class AFluidParticle*>& points);
	void forEachNearbyPoint(const FVector& origin, double radius);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;		
};
