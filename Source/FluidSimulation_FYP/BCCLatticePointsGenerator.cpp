// Fill out your copyright notice in the Description page of Project Settings.


#include "BCCLatticePointsGenerator.h"

void BCCLatticePointsGenerator::generate(const FVector& lowercorner, const FVector& uppercorner, double spacing, TArray<FVector>* points) const
{
	//Looks like we have an infinite recursive loop. 
	ForEachPoint(lowercorner, uppercorner, spacing, [&points](const FVector& point) {
		if (points->Num() > 1000)
		{
			UE_LOG(LogTemp, Warning, TEXT("Points array has more than 1000 items in it. ABORT ERROR!"));
			return false;
		}
		points->Add(point);
		return true;
		});
}

void BCCLatticePointsGenerator::ForEachPoint(const FVector& lowercorner, const FVector& uppercorner, double spacing, const ForEachBBCCallback& callback) const
{
	double halfSpacing = spacing / 2.0;
	double boxWidth = uppercorner.X - lowercorner.X;
	double boxHeight = uppercorner.Z - lowercorner.Z;
	double boxDepth = uppercorner.Y - lowercorner.Y;

	//Sometimes the dimensions are not generated correctly. 
	//UE_LOG(LogTemp, Warning, TEXT("Box Width: %f, Box Height: %f, Box Depth: %f"), boxWidth, boxHeight, boxDepth); //The box has dimension of 5.4 on all axis.

	FVector position;
	bool hasOffset = false;
	bool shouldQuit = false;
	for (int k = 0; k * halfSpacing <= boxDepth && !shouldQuit; ++k) {
		position.Y = k * halfSpacing + lowercorner.Y;

		double offset = (hasOffset) ? halfSpacing : 0.0;

		for (int j = 0; j * spacing + offset <= boxHeight && !shouldQuit; ++j) {
			position.Z = j * spacing + offset + lowercorner.Z;

			for (int i = 0; i * spacing + offset <= boxWidth; ++i) {
				position.X = i * spacing + offset + lowercorner.X;
				if (!callback(position)) {
					shouldQuit = true;
					break;
				}
			}
		}

		hasOffset = !hasOffset;
	}
}
