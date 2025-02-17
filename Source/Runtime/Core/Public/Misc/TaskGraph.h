#pragma once
#include "NameHandle.h"
#include "Task.h"

namespace Thunder
{
	inline std::atomic<uint32> GTaskUniqueID = 0;
	class ThreadPoolBase;

	class TGTaskNode : public ITask
	{
	public:
		TGTaskNode(const String& InDebugName = "")
			: DebugName(InDebugName)
		{
			UniqueId = GTaskUniqueID.fetch_add(1, std::memory_order_acq_rel);
		}

		NameHandle GetName() const
		{
			return DebugName;
		}
		
		uint32 UniqueId;
	private:
		NameHandle DebugName;
	};

    struct FTaskTable
    {
        FTaskTable(TGTaskNode* InTask)
            : Task(InTask)
        {}
        FTaskTable() = delete;
        TGTaskNode* Task;
        TArray<uint32> Predecessor {};
        TArray<uint32> Successor {};
    	std::atomic<uint32> PredecessorNum {0};
    };

	class TaskGraphProxy
	{
	public:
		TaskGraphProxy(ThreadPoolBase* InThreadPool)
			: ThreadPool(InThreadPool) {}

		~TaskGraphProxy()
		{
			WaitForCompletion();
		}

		void PushTask(TGTaskNode* Task, std::vector<uint32> PrepositionList = {});

		void Submit();

		void Reset(); // 等待前序任务完成,重置TG

	private:
		
		void WaitForCompletion() const; // 等待任务完成,且线程退出
	
	private:
		ThreadPoolBase* ThreadPool {};
		TArray<TGTaskNode*> TaskList {};
		THashMap<uint32, FTaskTable*> TaskDependencyMap {};

		// cv
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<uint32> TaskCount = 0;
	};



    
}
