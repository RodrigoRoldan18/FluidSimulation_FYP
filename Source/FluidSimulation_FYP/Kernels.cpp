// Fill out your copyright notice in the Description page of Project Settings.


#include "Kernels.h"

FSphStdKernel::FSphStdKernel() : h(0), h2(0), h3(0), h5(0) {}
FSphStdKernel::FSphStdKernel(double kernelRadius) : h(kernelRadius), h2(h* h), h3(h2* h), h5(h2* h3) {}
FSphStdKernel::FSphStdKernel(const FSphStdKernel& other) : h(other.h), h2(other.h2), h3(other.h3), h5(other.h5) {}
double FSphStdKernel::operator()(double distance) const
{
	//distance between particles (r)
	if (distance * distance >= h * h)
	{
		return 0.0;
	}
	else
	{
		double x = 1.0 - distance * distance / h2;
		return 315.0 / (64.0 * 3.14 * h3) * x * x * x;
	}
}

double FSphStdKernel::FirstDerivative(double distance) const
{
	if (distance >= h)
	{
		return 0.0;
	}
	else
	{
		double x = 1.0 - distance * distance / h2;
		return -945.0 / (32.0 * 3.14 * h5) * distance * x * x;
	}
}

double FSphStdKernel::SecondDerivative(double distance) const
{
	if (distance * distance >= h2)
	{
		return 0.0;
	}
	else
	{
		double x = distance * distance / h2;
		return 945.0 / (32.0 * 3.14 * h5) * (1 - x) * (3 * x - 1);
	}
}

FVector FSphStdKernel::Gradient(double distance, const FVector& directionToCentre) const
{
	return -FirstDerivative(distance) * directionToCentre;
}

//--------------------------------------------------------------------------------------

FSphSpikyKernel::FSphSpikyKernel() : h(0), h2(0), h3(0), h4(0), h5(0) {}
FSphSpikyKernel::FSphSpikyKernel(double kernelRadius) : h(kernelRadius), h2(h* h), h3(h2* h), h4(h2* h2), h5(h3* h2) {}

double FSphSpikyKernel::operator()(double distance) const
{
	if (distance >= h)
	{
		return 0.0;
	}
	else
	{
		double x = 1.0 - distance / h;
		return 15.0 / (3.14 * h3) * x * x * x;
	}
}

double FSphSpikyKernel::FirstDerivative(double distance) const
{
	if (distance >= h)
	{
		return 0.0;
	}
	else
	{
		double x = 1.0 - distance / h;
		return -45.0 / (3.14 * h4) * x * x;
	}
}

double FSphSpikyKernel::SecondDerivative(double distance) const
{
	if (distance >= h)
	{
		return 0.0;
	}
	else
	{
		double x = 1.0 - distance / h;
		return 90.0 / (3.14 * h5) * x;
	}
}

FVector FSphSpikyKernel::Gradient(double distance, const FVector& directionToCentre) const
{
	return -FirstDerivative(distance) * directionToCentre;
}