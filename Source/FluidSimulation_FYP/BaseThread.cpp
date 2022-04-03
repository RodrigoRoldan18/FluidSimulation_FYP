// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseThread.h"
#include "FluidSimulation_FYPGameModeBase.h"

FBaseThread::FBaseThread(int32 numOfCalculations, AFluidSimulation_FYPGameModeBase* gameMode)
{
	if (numOfCalculations > 0 && gameMode)
	{
		m_numOfCalculations = numOfCalculations;
		m_gameMode = gameMode;
	}
}

bool FBaseThread::Init()
{
	bStopThread = false;

	m_calcCount = 0;

	return true;
}

uint32 FBaseThread::Run()
{
	while (!bStopThread)
	{
		if (m_calcCount < m_numOfCalculations)
		{
			// do the thread calculations here
			m_currentCalculation += FMath::RandRange(20, 400);
			m_currentCalculation *= FMath::RandRange(2, 500);
			m_currentCalculation -= FMath::RandRange(10, 500);

			//add the result to the queue
			m_gameMode->ThreadQueue.Enqueue(m_currentCalculation);

			m_calcCount++;
		}
		else
		{
			bStopThread = true;
		}
	}
	return 0;
}

void FBaseThread::Stop()
{

}

///////////////////////////////////////////////////

void FOtherBaseThread::DoWork()
{

}
