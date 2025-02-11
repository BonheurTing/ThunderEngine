#pragma optimize("", off)
#include "CommonUtilities.h"
#include "HAL/Thread.h"
#include "Memory/MallocMinmalloc.h"
#include "Misc/TheadPool.h"
#include "Module/ModuleManager.h"
#include "Misc/Task.h"
#include "Templates/FunctionMy.h"

using namespace Thunder;

#pragma region TestWorkerThread

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

void TestWorkerThread()
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

#pragma endregion

void TestTFunction()
{
	// todo TFunction自己实现并跑通三种测试用例：lambda表达式，仿函数，bind
	// 目前来说根本没法用，问题太多了
	// 测试1: Lambda表达式赋值给TFunction，然后调用
	{
		TFunctionMy<int(float, std::string)> testFunction = [](float a, std::string b) -> int
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

#pragma region TestTask

class ExampleTask
{
public:
	friend class TTask<ExampleTask>;

	int32 Data;
	ExampleTask(): Data(0) {}

	ExampleTask(int32 InExampleData)
	 : Data(InExampleData) {}

	void DoWork() const
	{
		LOG("Execute TTask<ExampleTask>(Data: %d) with thread: %lu", Data, __threadid());
	}
};

void TestTask()
{
	// example 1
	const TFunction testFunction = []() -> void
	{
		LOG("Execute Function<void()> with thread: %lu", __threadid());
	};
	//FTask* ExampleTask1 = new FTask(testFunction);
	FTask* ExampleTask1 = new (TMemory::Malloc<FTask>()) FTask{testFunction};
	ExampleTask1->DoWork();

	// example 2
	const auto ExampleTask2 = new (TMemory::Malloc<ExampleInheritedTask>()) ExampleInheritedTask(1);
	ExampleTask2->DoWork();

	// example 3
	const auto ExampleTask3 = new (TMemory::Malloc<TTask<ExampleTask>>()) TTask<ExampleTask>(1);
	ExampleTask3->DoWork();
}

#pragma endregion

void TestIThread()
{
	// 测试1: 创建线程，执行任务
	{
		const auto testAsyncTask = new (TMemory::Malloc<TTask<ExampleTask>>()) TTask<ExampleTask>(2025);

		const auto testThreadProxy1 = new ThreadProxy(); //后面考虑分单个线程还是线程池的情况
		testThreadProxy1->CreateSingleThread();
		testThreadProxy1->PushTask(testAsyncTask);

		testThreadProxy1->WaitForCompletion(); // 线程结束
	}

	// 测试2: 创建线程，连续派发任务，执行任务
	{
		const auto testThreadProxy2 = new ThreadProxy();
		testThreadProxy2->CreateSingleThread();

		for (int i = 0; i < 5; i++)
		{
			const auto testAsyncTask = new (TMemory::Malloc<TTask<ExampleTask>>()) TTask<ExampleTask>(i);
			testThreadProxy2->PushTask(testAsyncTask);
		}
		
		testThreadProxy2->WaitForCompletion(); // 线程结束
	}
}


static int CallCount = 0;
class ExampleTask2
{
public:
	friend class TTask<ExampleTask2>;

	int32 ExampleData;
	ExampleTask2(): ExampleData(0) {}

	ExampleTask2(int32 InExampleData)
	 : ExampleData(InExampleData) {}

	void DoWork() const
	{
		CallCount++;
		LOG("ExampleData Pow Data: %d = %d, Count = %d，ThreadID: %d", ExampleData, ExampleData * ExampleData, CallCount, FPlatformTLS::GetCurrentThreadId());
	}
};



void TestThreadPool()
{

	IThreadPool* WorkerThreadPool = IThreadPool::Allocate();
	WorkerThreadPool->Create(8, 96 * 1024);


	// 测试1: 测试添加任务并执行
	/*{
		const auto TestAsyncTask = new (TMemory::Malloc<TTask<ExampleTask2>>()) TTask<ExampleTask>(214);
		WorkerThreadPool->AddQueuedWork(TestAsyncTask);
	}*/

	// 测试2: 添加多个任务，乱序执行
	{
		for (int i = 0; i < 10; i++)
		{
			const auto testAsyncTask = new (TMemory::Malloc<TTask<ExampleTask>>()) TTask<ExampleTask>(i);
			WorkerThreadPool->AddQueuedWork(testAsyncTask);
		}
	}

	// 测试2: ParallelFor
			
	/*std::vector<BoundingBox> ObjectsBounding(1024);
	std::vector<bool> CullResult(1024);
	
	WorkerThreadPool->ParallelFor([&CullResult, ObjectsBounding](uint32 bundleBegin, uint32 bundleSize)
	{
		for (int i = bundleBegin; i < bundleBegin + bundleSize; i++)
		{
			CullResult[i] = CullObject(ObjectsBounding[i]);
			LOG("Execute CullResult[%d]", i);
		}
	}, 1024, 256);*/

	WorkerThreadPool->WaitForCompletion();
}

int main()
{
	GMalloc = new TMallocMinmalloc();
	//TestWorkerThread(); // successful
	//TestTFunction(); // failed
	//TestTask(); // successful
	//TestIThread(); // successful
	TestThreadPool(); // successful
}


/**
* 先尝试一下伪代码设计
* 
* 1.创建线程池，指定线程数
* 2.用户侧添加任务，实现测是无锁队列，分发线程执行或者物理线程申请任务时不用加锁
* 3.线程池内部维护一个线程数组，每个线程有一个任务队列
* 4.
**/
