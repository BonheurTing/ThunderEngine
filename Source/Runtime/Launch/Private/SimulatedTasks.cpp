
#include "SimulatedTasks.h"
#include "PlatformProcess.h"
#include "Misc/TraceProfile.h"
#include "Concurrent/ConcurrentBase.h"

namespace Thunder
{
	void SimulatedPhysicsTask::DoWorkInner()
	{
		ThunderZoneScopedN("Physics");
		FPlatformProcess::BusyWaiting(1000);
	}

	void SimulatedCullTask::DoWorkInner()
	{
		ThunderZoneScopedN("Cull");
		FPlatformProcess::BusyWaiting(1000);
	}

	void SimulatedTickTask::DoWorkInner()
	{
		ThunderZoneScopedN("Tick");
		FPlatformProcess::BusyWaiting(1000);
	}

	void SimulatedAddMeshBatchTask::DoWork()
	{
		ThunderZoneScopedN("AddMeshBatch");
		FPlatformProcess::BusyWaiting(50);
		Dispatcher->Notify();
	}

	void SimulatedPopulateCommandList::DoWork()
	{
		ThunderZoneScopedN("PopulateCommandList");
		FPlatformProcess::BusyWaiting(50);
		Dispatcher->Notify();
	}
}
