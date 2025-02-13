#pragma once
#include "Assertion.h"
#include "CoreMinimal.h"
#include "NameHandle.h"
#include "Task.h"
#include "TheadPool.h"
#include "Templates/ThunderTemplates.h"


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
		Ready = 1,
		Completed = 2
	};

    struct FTaskTable
    {
        FTaskTable(TGTaskNode* InTask)
            : Task(InTask)
        {}
        FTaskTable() = delete;
        TGTaskNode* Task;
        std::vector<uint32> Preposition;
        std::vector<uint32> Postposition;
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

		void WaitForCompletion() const
		{
			ThreadPool->WaitForCompletion();
		}
	private:
		
		TGTaskNode* FindWork();

		void PrintTaskGraph() const;

	private:
		class ThreadPoolBase* ThreadPool {};
		std::vector<TGTaskNode*> TaskList {};
		std::unordered_map<uint32, FTaskTable*> TaskDependencyMap {};
	};



    
}
