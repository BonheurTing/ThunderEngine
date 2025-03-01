#include "Concurrent/ConcurrentBase.h"
#include "HAL/Event.h"

namespace Thunder
{
	void TaskDispatcher::OnCompleted()
	{
		LOG("TaskCounter OnCompleted");
		Event->Trigger();
	}
}
