// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class FLUIDSIMULATION_FYP_API BCCLatticePointsGenerator
{
	//Body-centered lattice points generator.
	//see http://en.wikipedia.org/wiki/Cubic_crystal_system
	//	  http://mathworld.wolfram.com/CubicClosePacking.html

public:
	typedef TFunction<bool(const FVector&)> ForEachBBCCallback;

	BCCLatticePointsGenerator() = default;
	~BCCLatticePointsGenerator() = default;

	//Generates points inside given range with spacing as the target point.
	void generate(const FVector& lowercorner, const FVector& uppercorner, double spacing, TArray<FVector>* points) const;

	//Invokes callback for each BCC-lattice points inside the bounds where spacing is the size of the unit cell of BCC structure.
	void ForEachPoint(const FVector& lowercorner, const FVector& uppercorner, double spacing, const ForEachBBCCallback& callback) const;
};
