#pragma once
#include "Concurrent/TaskGraph.h"

namespace Thunder
{
	class TaskDispatcher;

	class SimulatedPhysicsTask : public TaskGraphTask
	{
	public:
		int32 Data;
		SimulatedPhysicsTask(): Data(0)
		{
		}

		SimulatedPhysicsTask(int32 InPhysicsTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InPhysicsTaskData)
		{
		}

	private:
		void DoWorkInner() override;
	};

	class SimulatedCullTask : public TaskGraphTask
	{
	public:
		int32 Data;
		SimulatedCullTask(): Data(0)
		{
		}

		SimulatedCullTask(int32 InCullTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InCullTaskData)
		{
		}

	private:
		void DoWorkInner() override;
	};

	class SimulatedTickTask final : public TaskGraphTask
	{
	public:
		int32 Data;
		SimulatedTickTask(): Data(0)
		{
		}

		SimulatedTickTask(int32 InTickTaskData, const String& InDebugName = "")
			: TaskGraphTask(InDebugName)
			, Data(InTickTaskData)
		{
		}

	private:
		void DoWorkInner() override;
		
	};

}
