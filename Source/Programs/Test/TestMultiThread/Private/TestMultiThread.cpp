#pragma optimize("", off)
#include "CommonUtilities.h"
#include "HAL/Thread.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/TheadPool.h"
#include "Misc/AsyncTask.h"
#include "Misc/FTaskGraphInterface.h"
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


// 定义一个一次性任务
class FGraphTaskSimple
{
public:
	FGraphTaskSimple(const String& TheName, int InSomeArgument, float InWorkingTime = 1.0f)
		: TaskName(TheName), SomeArgument(InSomeArgument), WorkingTime(InWorkingTime) //
	{
		Log(__FUNCTION__);
	}

	~FGraphTaskSimple()
	{
		Log(__FUNCTION__);
	}

	// AnyThread中运行
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	// FireAndForget：一次性任务，没有依赖关系
	static ESubsequentsMode GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}

	// 执行任务
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		// The arguments are useful for setting up other tasks. 
		// Do work here, probably using SomeArgument.
		LOG("Succeed to Exacute Simple Task，ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
		FPlatformProcess::Sleep(WorkingTime);
		Log(__FUNCTION__);
	}

public:
	// 自定义参数
	String TaskName;
	int SomeArgument;
	float WorkingTime;

	// 日志接口
	void Log(const char* Action)
	{
		uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		String CurrentThreadName = ""; //FThreadManager::Get().GetThreadName(CurrentThreadId);
		LOG("%s@%s[%d] - %s, SomeArgument=%d", TaskName.c_str(), CurrentThreadName.c_str(),
			   CurrentThreadId,
			   Action, SomeArgument);
	}
};


// 定义一个支持依赖关系的任务
class FTask : public FGraphTaskSimple
{
public:
	using FGraphTaskSimple::FGraphTaskSimple;

	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	// TrackSubsequents - 支持依赖检查
	static ESubsequentsMode GetSubsequentsMode()
	{
		return TrackSubsequents;
	}
};



inline void Test_GraphTask_Simple()
{
	// 创建一个一次性任务并执行，任务结束自动删除
	auto Event = TGraphTask<FGraphTaskSimple>::CreateTask().ConstructAndDispatchWhenReady("SimpleTask1", 10000, 3);
	TAssert(!Event->IsComplete());

	// 创建一个任务但不放入TaskGraph执行
	/*TGraphTask<FGraphTaskSimple>* Task = TGraphTask<FGraphTaskSimple>::CreateTask().ConstructAndHold("SimpleTask2", 20000);
	if (Task)
	{
		// Unlock操作，放入TaskGraph执行
		LOG("Task is Unlock to Run...");
		Task->Unlock();
		Task = nullptr;
	}*/
	Event->Wait(ENamedThreads::AnyThread);  
}

// TaskA -> TaskB -> TaskC
inline void Test_GraphTask_Simple2()
{
	LOG("Start ......");

	FGraphEventRef TaskA, TaskB, TaskC;

	// TaskA
	{
		TaskA = TGraphTask<FTask>::CreateTask().ConstructAndDispatchWhenReady("TaksA", 1, 3);
	}

	// TaskA -> TaskB
	{
		FGraphEventArray Prerequisites;
		Prerequisites.push_back(TaskA);
		TaskB = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady("TaksB", 2, 2);
	}

	// TaskB -> TaskC
	{
		FGraphEventArray Prerequisites{TaskB};
		TaskC = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady("TaksC", 3, 1);
	}

	LOG("Construct is Done ......");

	// Waiting until TaskC is Done
	// FTaskGraphInterface::Get().WaitUntilTaskCompletes(TaskC);
	// Or.
	TaskC->Wait();

	LOG("TaskA is Done : %d", TaskA->IsComplete());
	LOG("TaskB is Done : %d", TaskA->IsComplete());
	LOG("TaskC is Done : %d", TaskC->IsComplete());
	LOG("Over ......");
}


int main()
{
	GMalloc = new TMallocMinmalloc();

	FTaskGraphInterface::Startup(6);

	// 当前线程Attach为GameThread
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);
	
	Test_GraphTask_Simple();

	//Test_GraphTask_Simple2();


	LOG("Ending the test...");
}


int Example()
{
	GMalloc = new TMallocMinmalloc();

	// TFunction example 1.
	{
		TFunction<int(float, std::string)> testFunction = [](float a, std::string b) -> int
		{
			LOG("a: %f, b: %s", a, b.c_str());
			return 0;
		};
		int dummy = testFunction(4.0f, "Hello World function");
		dummy = Thunder::AsyncTask(testFunction, 5.0f, std::string("Hello World Invoke"));
	}
	/*
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
	}

	// IThreadPool example 1.
	LOG("IThreadPool example 1 start");
	TArray<ITask*> testTasks = {};
	for (int i = 0; i < 20; i++)
	{
		const auto testAsyncTask = new FAsyncTask<ExampleAsyncTask2>(i);
		testTasks.push_back(testAsyncTask);
	}
	IThreadPool::ParallelFor(testTasks, EThreadPriority::Normal, 8);
	*/


	/*// 初始化整个TaskGraph系统
	FTaskGraphInterface::Startup(6);
	// 当前线程附加到AnyThread
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::AnyThread);
	

	// Task queue example 1.
	// 创建一个任务并在后台AnyThread中执行
	FGraphEventRef Event = FunctionGraphTask::CreateAndDispatchWhenReady([]()
		{
			FPlatformProcess::Sleep(5.f); // pause for a bit to let waiting start
			LOG("Succeed to Graph Task，ThreadID: %d", FPlatformTLS::GetCurrentThreadId());
		}
	);
	//TAssert(!Event->IsComplete());

	// 在主线程中等待该任务完成
	Event->Wait(ENamedThreads::GameThread);

	
	
	// Task queue example 2.
	// 同时创建多个任务
	FGraphEventArray Tasks;
	for (int i = 0; i < 10; ++i)
	{
		Tasks.push_back(FunctionGraphTask::CreateAndDispatchWhenReady([i]()
		{
			
		}));
	}

	// Task Graph example 1.
	
	// 在主线程中等待所有任务完成
	FTaskGraphInterface::Get().WaitUntilTasksComplete(MoveTemp(Tasks), ENamedThreads::GameThread);

	FGraphEventRef TaskA, TaskB, TaskC, TaskD, TaskE;

	// TaskA
	TaskA = TGraphTask<FTask>::CreateTask().ConstructAndDispatchWhenReady(TEXT("TaksA"), 1, 1);

	// TaskB 依赖 TaskA
	{
		FGraphEventArray Prerequisites;
		Prerequisites.push_back(TaskA);
		TaskB = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady(TEXT("TaksB"), 1, 1);
	}

	// TaskC 依赖 TaskB
	{
		FGraphEventArray Prerequisites;
		Prerequisites.push_back(TaskB);
		TaskC = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady(TEXT("TaksC"), 1, 1);
	}

	// TaskD 依赖 TaskA
	{
		FGraphEventArray Prerequisites;
		Prerequisites.push_back(TaskA);
		TaskD = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady(TEXT("TaksD"), 1, 3);
	}

	// TaskE 依赖 TaskC、TaskD
	{
		FGraphEventArray Prerequisites {TaskC, TaskD};
		TaskE = TGraphTask<FTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady(TEXT("TaksE"), 1, 1);
	}

	// 在当前线程等待，直到TaskE完成
	TaskE->Wait();*/
	


	//------------------------------------------------------------------------------------------------------
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
	TaskManager->PushTaskGraph(NameHandle("IOThreadPool"), taskGraph, ETaskPriority::Normal);


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

	//system("pause");
	return 0;
}


