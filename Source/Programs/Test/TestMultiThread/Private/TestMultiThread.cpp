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

	int32 FrameData;
	ExampleAsyncTask1(): FrameData(0)
	{
	}

	ExampleAsyncTask1(int32 InExampleData)
	 : FrameData(InExampleData)
	{
	}

	void DoWork()
	{
		LOG("ExampleData Add 1: %d", FrameData + 1);
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

std::queue<std::string> taskQueue; // 任务队列
std::mutex mtx;                    // 互斥锁，保护任务队列
std::condition_variable cv;        // 条件变量，用于线程同步
bool stop = false;                 // 标志位，用于停止子线程

// 子线程函数
void WorkerThread() {
	while (true) {
		std::unique_lock<std::mutex> lock(mtx);

		// 等待任务队列非空或停止信号
		cv.wait(lock, [] { return !taskQueue.empty() || stop; });

		// 如果收到停止信号且队列为空，退出线程
		if (stop && taskQueue.empty()) {
			break;
		}

		// 取出任务
		std::string task = taskQueue.front();
		taskQueue.pop();
		lock.unlock(); // 提前释放锁，减少锁的持有时间

		// 执行任务
		std::cout << "work thread execute task: " << task << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟任务执行时间
	}
	std::cout << "work thread exit" << std::endl;
}

void TestLockFreeQueue()
{
	// 启动子线程
	std::thread worker(WorkerThread);

	// 主线程不断喂任务
	for (int i = 1; i <= 5; ++i) {
		std::string task = "task" + std::to_string(i);
		{
			std::lock_guard<std::mutex> lock(mtx);
			taskQueue.push(task);
			std::cout << "main thread push task: " << task << std::endl;
		}
		cv.notify_one(); // 通知子线程有新任务
		std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 模拟任务添加间隔
	}

	// 停止子线程
	{
		std::lock_guard<std::mutex> lock(mtx);
		stop = true;
	}
	cv.notify_one(); // 通知子线程退出

	// 等待子线程结束
	worker.join();
	std::cout << "main thread exit" << std::endl;
}

void TestTFunction()
{
	// todo TFunction自己实现并跑通三种测试用例：lambda表达式，仿函数，bind
	
	// 测试1: Lambda表达式赋值给TFunction，然后调用
	{
		TFunction<int(float, std::string)> testFunction = [](float a, std::string b) -> int
		{
			LOG("a: %f, b: %s", a, b.c_str());
			return 0;
		};
		const int dummy = testFunction(4.0f, "Hello World function");
		Thunder::AsyncTask(testFunction, static_cast<float>(dummy), std::string("Hello World Invoke"));
	}
	/*
	// 测试2: 仿函数对象赋值给TFunction，然后调用
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
		TestCallable testCallable{ 6 }; // capture数据
		TFunction<int(int, std::string)> testFunction2 = testCallable;
		int dummy = testFunction2(4, "Hello World function");
	}
	
	// 测试3: bind赋值给TFunction，然后调用
	auto const testLambda = [](float a, std::string b, int c, double d) -> int
	{
		LOG("a: %f, b: %s, c: %d, d: %f", a, b.c_str(), c, d);
		return 0;
	};
	TFunction<int(float, double)> testFunction3 = std::bind(testLambda, std::placeholders::_1, "Hello~", 7);
	Thunder::Invoke(testFunction3, 1.0, 2.0);
	testFunction3->Invoke(1.0, 2.0);
	*/
}

void TestTask()
{
	
}


void TestIThread()
{
	
}


void TestThreadPool()
{
	
}

int MultiThreadExample()
{
	
	// no thread example 1.
	{
		FAsyncTask<ExampleAsyncTask1>* NoThreadTask = new FAsyncTask<ExampleAsyncTask1>(1);
		NoThreadTask->StartTask();
		delete NoThreadTask;
	}

	// IThread example 1.
	{
		// 测试单个任务，在thread proxy中执行
		FAsyncTask<ExampleAsyncTask1>* testAsyncTask = new FAsyncTask<ExampleAsyncTask1>(2);
		ThreadProxy* testThreadProxy1 = new ThreadProxy();
		testThreadProxy1->Create(nullptr, 4096); //内部用 IThread实现
		testThreadProxy1->PushAndExecuteTask(testAsyncTask);
	}

	/*// IThread example 2.
	{
		// 测试创建线程；实际上线程的创建不应该暴露出来，ThreadProxy创建内部是IThread的创建
		ThreadProxy* testThreadProxy2 = new ThreadProxy();
		IThread* testThread2 = IThread::Create(testThreadProxy2, "Test", 0, EThreadPriority::Normal);
		testThread2->WaitForCompletion();
		delete testThreadProxy2;
		delete testThread2;
	}*/

	// IThread example 3
	{
		// 测试多次喂task执行
		ThreadProxy* testThreadProxy3 = new ThreadProxy();
		testThreadProxy3->Create(nullptr, 4096); //内部用 IThread实现
		for (int i = 0; i < 5; i++)
		{
			FAsyncTask<ExampleAsyncTask1>* testRenderThreadTask = new FAsyncTask<ExampleAsyncTask1>(i);
			testThreadProxy3->PushAndExecuteTask(testRenderThreadTask);
			//Sleep(500);
			//delete testRenderThreadTask;
		}
		//testThreadProxy3->WaitForCompletion();
		//delete testThreadProxy3;
	}
	
	// IThreadPool example 1.
	/*LOG("IThreadPool example 1 start");
	TArray<ITask*> testTasks = {};
	for (int i = 0; i < 20; i++)
	{
		const auto testAsyncTask = new FAsyncTask<ExampleAsyncTask2>(i);
		testTasks.push_back(testAsyncTask);
	}
	IThreadPool::ParallelFor(testTasks, EThreadPriority::Normal, 8);*/

	//system("pause");
	return 0;
}

int main()
{
	GMalloc = new TMallocMinmalloc();
	TestLockFreeQueue();
	//TestTFunction();
}
