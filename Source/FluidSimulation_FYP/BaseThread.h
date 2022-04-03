// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "Core/Public/HAL/Runnable.h"

//Forward declarations
class FRunnableThread;
class AFluidSimulation_FYPGameModeBase;

/**
 * 
 */
class FLUIDSIMULATION_FYP_API FBaseThread : public FRunnable
{
	int32 m_numOfCalculations; // amount of calculations from the constructor
	int32 m_calcCount; // iterations of the calculations that we are doing
	AFluidSimulation_FYPGameModeBase* m_gameMode;
	int32 m_currentCalculation; // most recent calculation

public:
	FBaseThread(int32 numOfCalculations, AFluidSimulation_FYPGameModeBase* gameMode);
	~FBaseThread() = default;

	bool bStopThread;

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
};

//////////////////////////////////////////////

class FLUIDSIMULATION_FYP_API FOtherBaseThread : public FNonAbandonableTask
{
	friend class FAsyncTask<FOtherBaseThread>;

	int32 TestData;

public:
	FOtherBaseThread(int32 InTestData) : TestData(InTestData) {}	
	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FOtherBaseThread, STATGROUP_ThreadPoolAsyncTasks); }
	void DoWork();
};
