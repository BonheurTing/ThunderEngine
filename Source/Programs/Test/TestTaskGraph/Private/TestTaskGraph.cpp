#pragma optimize("", off)
#include "Misc/TaskGraph.h"

using namespace Thunder;

class ExampleAsyncTask1 : public TaskAllocator
{
public:

	int32 FrameData;
	ExampleAsyncTask1(): FrameData(0)
	{
	}

	ExampleAsyncTask1(int32 InExampleData, const String& InDebugName = "")
		: TaskAllocator(InDebugName)
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
	TaskGraphManager* TaskGraph = new TaskGraphManager();
	ExampleAsyncTask1* TaskA = new ExampleAsyncTask1(1, "TaskA");
	ExampleAsyncTask1* TaskB = new ExampleAsyncTask1(2, "TaskB");
	
	TaskGraph->PushTask(TaskA);
	TaskGraph->PushTask(TaskB, {TaskA->UniqueId});

	TaskGraph->CreateWorkerThread();
	TaskGraph->WaitForComplete();
}


