#pragma optimize("", off)
#include "HAL/Thread.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/TheadPool.h"
#include "Misc/AsyncTask.h"
#include "Module/ModuleManager.h"

using namespace Thunder;


class ExampleAsyncTask1
{
public:
	friend class FAsyncTask<ExampleAsyncTask1>;

	int32 ExampleData;
	ExampleAsyncTask1(): ExampleData(0)
	{
	}

	ExampleAsyncTask1(int32 InExampleData)
	 : ExampleData(InExampleData)
	{
	}

	void DoWork()
	{
		LOG("ExampleData Add 1: %d", ExampleData + 1);
	}
};
static int CallCount = 0;
class ExampleAsyncTask2
{
public:
	friend class FAsyncTask<ExampleAsyncTask2>;

	int32 ExampleData;
	ExampleAsyncTask2(): ExampleData(0)
	{
	}

	ExampleAsyncTask2(int32 InExampleData)
	 : ExampleData(InExampleData)
	{
	}

	void DoWork()
	{
		CallCount++;
		LOG("ExampleData Pow Data: %d = %d, Count = %d，ThreadID: %d", ExampleData, ExampleData * ExampleData, CallCount, FPlatformTLS::GetCurrentThreadId());
	}
};


int main()
{
	GMalloc = new TMallocMinmalloc();

	// TFunction example 1.
	/*{
		TFunction<int(float, std::string)> testFunction = [](float a, std::string b) -> int
		{
			LOG("a: %f, b: %s", a, b.c_str());
			return 0;
		};
		int dummy = testFunction(4.0f, "Hello World function");
		dummy = Thunder::Invoke(testFunction, 5.0f, std::string("Hello World Invoke"));
	}

	// no thread example 1.
	{
		FAsyncTask<ExampleAsyncTask1>* NoThreadTask = new FAsyncTask<ExampleAsyncTask1>(1);
		NoThreadTask->StartTask();
		NoThreadTask->EnsureCompletion();
		delete NoThreadTask;
	}
	
	// IThread example 1.
	{
		// 测试单个任务，在thread proxy中执行
		FAsyncTask<ExampleAsyncTask1>* testAsyncTask = new FAsyncTask<ExampleAsyncTask1>(2);
		ThreadProxy* testThreadProxy1 = new ThreadProxy();
		testThreadProxy1->PushTask(testAsyncTask);
		testThreadProxy1->Create(nullptr, 4096, EThreadPriority::Normal); //内部用 IThread实现
		testThreadProxy1->KillThread();
		delete testAsyncTask;
		delete testThreadProxy1;
	}

	// IThread example 2.
	{
		// 测试创建线程；实际上线程的创建不应该暴露出来，ThreadProxy创建内部是IThread的创建
		ThreadProxy* testThreadProxy2 = new ThreadProxy();
		IThread* testThread2 = IThread::Create(testThreadProxy2, "Test", 0, EThreadPriority::Normal);
		testThread2->WaitForCompletion();
		delete testThreadProxy2;
		delete testThread2;
	}*/

	// IThreadPool example 1.
	LOG("IThreadPool example 1 start");
	TArray<ITask*> testTasks = {};
	for (int i = 0; i < 20; i++)
	{
		const auto testAsyncTask = new FAsyncTask<ExampleAsyncTask2>(i);
		testTasks.push_back(testAsyncTask);
	}
	IThreadPool::ParallelFor(testTasks, EThreadPriority::Normal, 8);

	

//------------------------------------------------------------------------------------------------------
	/*
	// TFunction example 2.
	{
		struct TestCallable
		{
			TestCallable(int capturedInteger) : capturedInteger(capturedInteger) {}

			int operator()(std::string b) const
			{
				LOG("a: %d, b: %s", capturedInteger, b.c_str());
				return 0;
			}

		private:
			int capturedInteger = 0;
		};
		TestCallable testCallable{ 6 };
		TFunction<int(int, std::string)> testFunction2 = testCallable;
		int dummy = testFunction2("Hello World");
		//Thunder::Invoke(testFunction2, std::string("Hello World"));
	}
	

	
	// TFunction example 3.
	auto const testLambda = [](float a, std::string b, int c, double d) -> int
	{
		LOG("a: %f, b: %s, c: %d, d: %f", a, b.c_str(), c, d);
		return 0;
	};
	TFunction<int(float, double)> testFunction3 = std::bind(testLambda, std::placeholders::_1, "Hello~", 7);
	Thunder::Invoke(testFunction3, 1.0, 2.0);
	testFunction3->Invoke(1.0, 2.0);*/
	
	/*
	
	
	

	// Task queue example 1.
	TaskManager->AttachThread("Main", GameThread, EThreadPriority::Normal);
	TaskManager->PushQueuedTask(NameHandle("Main"),
		[]()
		{
			// Do some work.
		}, ETaskPriority::Normal);

	// Task queue example 2.
	TaskManager->AttachThreadPool("IOThreadPool", threadPool, EThreadPriority::Low);
	TaskManager->PushQueuedTask(NameHandle("IOThreadPool"),
		[]()
		{
			// Do some work.
		}, ETaskPriority::Normal);

	// Task graph example 1.
	ITask* task1 = new Task([](){});
	ITask* task2 = new Task([](){});
	task2->AddDependency(task1);
	ITask* task3 = new Task([](){});
	task3->AddDependency(task1);
	ITask* startTask = new Task([](){});
	startTask->AddDependency(task2);
	startTask->AddDependency(task3);
	ITaskGraph* taskGraph = new TaskGraph();
	taskGraph->SetRootNode(startTask);
	TaskManager->PushTaskGraph(NameHandle("IOThreadPool"), taskGraph, ETaskPriority::Normal);*/

	//system("pause");
	return 0;
}
#pragma optimize("", on)