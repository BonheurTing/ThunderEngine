#pragma optimize("", off)
#include "Memory/MallocMinmalloc.h"
#include "Misc/TaskGraph.h"

using namespace Thunder;

class ExampleAsyncTask1 : public TGTaskNode
{
public:

	int32 FrameData;
	ExampleAsyncTask1(): FrameData(0)
	{
	}

	ExampleAsyncTask1(int32 InExampleData, const String& InDebugName = "")
		: TGTaskNode(InDebugName)
		, FrameData(InExampleData)
	{
	}

	void DoWork()
	{
		LOG("ExampleData Add 1: %d", FrameData + 1);
	}
};

void main()
{
	GMalloc = new TMallocMinmalloc();
	
	ThreadPoolBase* WorkerThreadPool = new ThreadPoolBase();
	WorkerThreadPool->Create(8, 96 * 1024);
	
	TaskGraphProxy* TaskGraph = new TaskGraphProxy(WorkerThreadPool);
	
	ExampleAsyncTask1* TaskA = new ExampleAsyncTask1(1, "TaskA");
	ExampleAsyncTask1* TaskB = new ExampleAsyncTask1(2, "TaskB");
	
	TaskGraph->PushTask(TaskA);
	TaskGraph->PushTask(TaskB, {TaskA->UniqueId});

	TaskGraph->Submit();
	TaskGraph->WaitForCompletion();
}

#pragma optimize("", on)
