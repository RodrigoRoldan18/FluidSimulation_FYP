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
public:
	typedef TFunction<void(size_t, const FVector&)> ForEachNearbyPointCallback;

private:
	double m_gridSpacing = 2.0; //radius * 2
	FIntVector m_resolution = FIntVector(64, 64, 64); //the higher the resolution the better
	TArray<TArray<size_t>> m_buckets;
	TArray<FVector> m_particlePositions;

	size_t getHashKeyFromPosition(const FVector& pos) const;
	FIntVector getBucketIndex(const FVector& pos) const;
	size_t getHashKeyFromBucketIndex(const FIntVector& bucketIndex) const;
	void getNearbyKeys(const FVector& pos, size_t* nearbyKeys) const;

public:	
	// Sets default values for this component's properties
	UNeighbourSearch();

	void initialiseNeighbourSearcher(const FIntVector& resolution, double gridSpacing);
	void build(const TArray<class AFluidParticle*>& points);
	void forEachNearbyPoint(const FVector& origin, double radius, const ForEachNearbyPointCallback& callback);	
};