#include "Concurrent/ConcurrentBase.h"
#include "HAL/Event.h"

namespace Thunder
{
	void TaskDispatcher::OnCompleted()
	{
		LOG("TaskDispatcher OnCompleted");
		Event->Trigger();
	}
}
