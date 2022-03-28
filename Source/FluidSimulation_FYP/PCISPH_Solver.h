// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParticleSystemSolver.h"
#include "PCISPH_Solver.generated.h"

/**
 * 
 */
UCLASS()
class FLUIDSIMULATION_FYP_API APCISPH_Solver : public AParticleSystemSolver
{
	GENERATED_BODY()

	double m_maxDensityErrorRatio{ 0.1 };
	unsigned int m_maxNumberOfIterations{ 5 };

	TArray<FVector> m_tempPositions;
	TArray<FVector> m_tempVelocities;
	TArray<FVector> m_tempPressureForces;
	TArray<double> m_densityErrors;
	
	double computeDelta(double timeStepInSeconds);
	double computeBeta(double timeStepInSeconds);
	void computePressureGradientForce(double timeStepInSeconds, const TArray<double>& densities);

protected:
	void onBeginAdvanceTimeStep() override;
	void accumulatePressureForce(double timeStepInSeconds) override;
	void accumulateForces(double timeStepInSeconds) override;
};
