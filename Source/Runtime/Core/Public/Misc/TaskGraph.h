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
			UniqueId = GTaskUniqueID.fetch_add(1);
		}

		NameHandle GetName() const
		{
			return DebugName;
		}
		
		uint32 UniqueId;
	private:
		NameHandle DebugName;
	};

	enum class ETaskState : uint8
	{
		Wait = 0,
		Issued = 1,
		Completed = 2
	};

    struct FTaskTable
    {
        FTaskTable(TGTaskNode* InTask)
            : Task(InTask)
        {}
        FTaskTable() = delete;
        TGTaskNode* Task;
        std::vector<uint32> Preposition {};
        std::vector<uint32> Postposition {};
        ETaskState State = ETaskState::Wait;//评估风险
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

		void WaitForCompletion() const; // 等待任务完成,且线程退出
		
	private:
		
		std::list<TGTaskNode*> FindWork();

		void PrintTaskGraph() const;

	private:
		class ThreadPoolBase* ThreadPool {};
		std::vector<TGTaskNode*> TaskList {};
		std::unordered_map<uint32, FTaskTable*> TaskDependencyMap {};

		// cv
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<uint32> TaskCount = 0;
	};



    
}
