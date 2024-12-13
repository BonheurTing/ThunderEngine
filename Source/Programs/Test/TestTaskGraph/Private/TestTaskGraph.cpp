#pragma optimize("", off)
#include "CommonUtilities.h"
#include "NameHandle.h"
#include "Misc/FTaskGraphInterface.h"


using namespace Thunder;

std::atomic<uint32> GTaskUniqueID = 0;

class TaskAllocator : public Noncopyable
{
public:
	TaskAllocator(const String& InDebugName = "")
		: DebugName(InDebugName)
	{
		UniqueId = GTaskUniqueID.fetch_add(1);
	}
	virtual  ~TaskAllocator() {}

	NameHandle GetName() const
	{
		return DebugName;
	}

	virtual void DoWork() = 0;
	
	uint32 UniqueId;

private:
	NameHandle DebugName;
};

class ExampleAsyncTask1 : public TaskAllocator
{
public:

	int32 ExampleData;
	ExampleAsyncTask1(): ExampleData(0)
	{
	}

	ExampleAsyncTask1(int32 InExampleData, const String& InDebugName = "")
		: TaskAllocator(InDebugName)
		, ExampleData(InExampleData)
	{
	}

	void DoWork()
	{
		LOG("ExampleData Add 1: %d", ExampleData + 1);
	}
};

struct FTaskTable
{
	FTaskTable(TaskAllocator* InTask)
		: Task(InTask)
	{}
	FTaskTable() = delete;
	TaskAllocator* Task;
	std::vector<uint32> Preposition;
	std::vector<uint32> Postposition;
	bool bIsComplete = false;
};

class TaskGraphManager
{
public:
	~TaskGraphManager()
	{
		WaitForComplete();
		CloseHandle(Thread);
	}
	
	void PushTask(TaskAllocator* Task, std::vector<uint32> PrepositionList = {})
	{
		TaskList.push_back(Task);
		FTaskTable* Table = new FTaskTable(Task);
		Table->Preposition = PrepositionList;
		TaskDependencyMap[Task->UniqueId] = Table;
		
		for (auto Preposition : PrepositionList)
		{
			TaskDependencyMap[Preposition]->Postposition.push_back(Task->UniqueId);
		}
	}

	uint32 CreateWorkerThread()
	{
		uint32 ThreadID;
		Thread = CreateThread(nullptr, 0, ThreadProc, this, 0
		, reinterpret_cast<::DWORD*>(&ThreadID)); //STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED
		if (Thread == nullptr)
		{
			TAssert(false);
		}
		else
		{
			LOG("ThreadID: %d", ThreadID);
			auto ret = ResumeThread(Thread);
			LOG("ResumeThread %d: %d", ret, GetLastError());
		}
		return Thread != nullptr;
	}

	void WaitForComplete()
	{
		WaitForSingleObject(Thread, INFINITE);
	}
private:
	static ::DWORD __stdcall ThreadProc(LPVOID pThis)
	{
		LOG("ThreadProc");
		TAssert(pThis);
		auto* ThisThread = static_cast<TaskGraphManager*>(pThis);
		return ThisThread->ExacuteImpl();
	}
	
	uint32 ExacuteImpl()
	{
		LOG("ExacuteImpl");
		while (auto Task = FindWork())
		{
			Task->DoWork();
			TaskDependencyMap[Task->UniqueId]->bIsComplete = true;
		}
		PrintTaskGraph();

		for (auto& Task : TaskList)
		{
			TAssert(TaskDependencyMap[Task->UniqueId]->bIsComplete);
			delete Task;
		}
		return TaskDependencyMap.size();
	}

	TaskAllocator* FindWork()
	{
		for (auto& Task : TaskList)
		{
			if (TaskDependencyMap[Task->UniqueId]->bIsComplete)
				continue;
			
			bool bHasPreposition = false;
			for (auto Pre : TaskDependencyMap[Task->UniqueId]->Preposition)
			{
				if (!TaskDependencyMap[Pre]->bIsComplete)
				{
					bHasPreposition = true;
					break;
				}
			}
			if (!bHasPreposition)
			{
				LOG("FindWork: %s", Task->GetName().c_str());
				return Task;
			}
		}
		return nullptr;
	}

	void PrintTaskGraph()
	{
		TAssert(TaskList.size() == TaskDependencyMap.size());
		String report = "Task Graph has " + std::to_string(TaskList.size()) + " Tasks.";
		for (auto pair : TaskDependencyMap)
		{
			report = report + " name: " + pair.second->Task->GetName().ToString() + ", bIsComplete: " + (pair.second->bIsComplete ? "1" : "0");
		}
	}

private:
	HANDLE Thread;
	std::vector<TaskAllocator*> TaskList;
	std::unordered_map<uint32, FTaskTable*> TaskDependencyMap; //
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


