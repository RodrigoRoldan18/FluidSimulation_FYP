// Fill out your copyright notice in the Description page of Project Settings.


#include "NeighbourSearch.h"
#include "FluidParticle.h"

size_t UNeighbourSearch::getHashKeyFromPosition(const FVector& pos) const
{
	FIntVector bucketIndex;

	//get the bucket index
	bucketIndex = getBucketIndex(pos);

	//get the hash key from the bucket index
	return getHashKeyFromBucketIndex(bucketIndex);
}

FIntVector UNeighbourSearch::getBucketIndex(const FVector& pos) const
{
	FIntVector bucketIndex;
	//need to round these values down
	bucketIndex.X = pos.X / m_gridSpacing;
	bucketIndex.Y = pos.Y / m_gridSpacing;
	bucketIndex.Z = pos.Z / m_gridSpacing;
	return bucketIndex;
}

size_t UNeighbourSearch::getHashKeyFromBucketIndex(const FIntVector& bucketIndex) const
{
	FIntVector wrappedIndex = bucketIndex;
	wrappedIndex.X = bucketIndex.X % m_resolution.X;
	wrappedIndex.Y = bucketIndex.Y % m_resolution.Y;
	wrappedIndex.Z = bucketIndex.Z % m_resolution.Z;

	if (wrappedIndex.X < 0) { wrappedIndex.X += m_resolution.X; }
	if (wrappedIndex.Y < 0) { wrappedIndex.Y += m_resolution.Y; }
	if (wrappedIndex.Z < 0) { wrappedIndex.Z += m_resolution.Z; }

	//there might be some issues with the way Unreal has its direction axis.
	return static_cast<size_t>((wrappedIndex.Z * m_resolution.Y + wrappedIndex.Y) * m_resolution.X + wrappedIndex.X);
}

void UNeighbourSearch::getNearbyKeys(const FVector& pos, size_t* nearbyKeys) const
{
	FIntVector originIndex = getBucketIndex(pos);
	FIntVector nearbyBucketIndices[8];

	for (int i = 0; i < 8; i++)
	{
		nearbyBucketIndices[i] = originIndex;
	}

	if ((originIndex.X + 0.5f) * m_gridSpacing <= pos.X)
	{
		nearbyBucketIndices[4].X += 1;
		nearbyBucketIndices[5].X += 1;
		nearbyBucketIndices[6].X += 1;
		nearbyBucketIndices[7].X += 1;
	}
	else
	{
		nearbyBucketIndices[4].X -= 1;
		nearbyBucketIndices[5].X -= 1;
		nearbyBucketIndices[6].X -= 1;
		nearbyBucketIndices[7].X -= 1;
	}

	if ((originIndex.Y + 0.5f) * m_gridSpacing <= pos.Y)
	{
		nearbyBucketIndices[2].Y += 1;
		nearbyBucketIndices[3].Y += 1;
		nearbyBucketIndices[6].Y += 1;
		nearbyBucketIndices[7].Y += 1;
	}
	else
	{
		nearbyBucketIndices[2].Y -= 1;
		nearbyBucketIndices[3].Y -= 1;
		nearbyBucketIndices[6].Y -= 1;
		nearbyBucketIndices[7].Y -= 1;
	}

	if ((originIndex.Z + 0.5f) * m_gridSpacing <= pos.Z)
	{
		nearbyBucketIndices[1].Z += 1;
		nearbyBucketIndices[3].Z += 1;
		nearbyBucketIndices[5].Z += 1;
		nearbyBucketIndices[7].Z += 1;
	}
	else
	{
		nearbyBucketIndices[1].Z -= 1;
		nearbyBucketIndices[3].Z -= 1;
		nearbyBucketIndices[5].Z -= 1;
		nearbyBucketIndices[7].Z -= 1;
	}

	//this might be wrong due to Unreal's change of direction axis.

	for (int i = 0; i < 8; i++)
	{
		nearbyKeys[i] = getHashKeyFromBucketIndex(nearbyBucketIndices[i]);
	}
}

// Sets default values for this component's properties
UNeighbourSearch::UNeighbourSearch()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UNeighbourSearch::initialiseNeighbourSearcher(const FIntVector& resolution, double gridSpacing)
{
	m_resolution = resolution;
	m_gridSpacing = gridSpacing;
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
	m_buckets.SetNumZeroed(m_resolution.X * m_resolution.Y * m_resolution.Z);
	m_particlePositions.Reserve(points.Num());

	//Put points into buckets
	for (size_t i = 0; i < points.Num(); i++)
	{
		m_particlePositions.Add(points[i]->GetParticlePosition());
		size_t key = getHashKeyFromPosition(m_particlePositions[i]);
		m_buckets[key].Push(i);
	}
}

void UNeighbourSearch::forEachNearbyPoint(const FVector& origin, double radius, const ForEachNearbyPointCallback& callback)
{
	if (m_buckets.Num() == 0)
	{
		return;
	}

	size_t nearbyKeys[8];
	getNearbyKeys(origin, nearbyKeys);

	const double queryRadiusSquared = radius * radius;

	for (int i = 0; i < 8; i++)
	{
		const auto& bucket = m_buckets[nearbyKeys[i]];
		size_t numberOfPointsInBucket = bucket.Num();

		for (size_t j = 0; j < numberOfPointsInBucket; ++j)
		{
			size_t pointIndex = bucket[j];
			double rSquared = (m_particlePositions[pointIndex] - origin).SizeSquared();
			if (rSquared <= queryRadiusSquared)
			{
				callback(pointIndex, m_particlePositions[pointIndex]);
			}
		}
	}
}