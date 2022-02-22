// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kernels.generated.h"

//This Gaussian-like kernel will be used for the interpolation and field calculation
USTRUCT()
struct FSphStdKernel
{
	GENERATED_BODY()

		//kernel radius
		double h, h2, h3, h5;

	FSphStdKernel();
	explicit FSphStdKernel(double kernelRadius);
	FSphStdKernel(const FSphStdKernel& other);
	double operator()(double distance) const;
	double FirstDerivative(double distance) const;
	double SecondDerivative(double distance) const;
	FVector Gradient(double distance, const FVector& directionToCentre) const;
};

//This Mueller's kernel will only be used for the gradient and Laplacian calculation
USTRUCT()
struct FSphSpikyKernel
{
	GENERATED_BODY()

		double h, h2, h3, h4, h5;

	FSphSpikyKernel();
	explicit FSphSpikyKernel(double kernelRadius);
	double operator()(double distance) const;
	double FirstDerivative(double distance) const;
	double SecondDerivative(double distance) const;
	FVector Gradient(double distance, const FVector& directionToCentre) const;
};