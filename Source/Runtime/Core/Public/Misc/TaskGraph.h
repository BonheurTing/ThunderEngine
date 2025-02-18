#pragma once
#include "NameHandle.h"
#include "Task.h"

namespace Thunder
{
	struct TaskGraphNode;
	class ThreadPoolBase;

	class TaskGraphTask : public ITask
	{
	public:
		TaskGraphTask(const String& InDebugName = "")
			: DebugName(InDebugName) {}

		TaskGraphTask(TFunction<void(TaskGraphTask*)>&& InFunc, const String& InDebugName = "")
			: CallBack(std::move(InFunc)), DebugName(InDebugName) {}

		NameHandle GetName() const
		{
			return DebugName;
		}

		TaskGraphNode* GetOwner() const { return NodeOwner; }

		void SetOwner(TaskGraphNode* InNode) { NodeOwner = InNode; }

		void SetCallBack(TFunction<void(TaskGraphTask*)>&& InFunc) { CallBack = std::move(InFunc); }

		virtual void DoWork() final
		{
			DoWorkInner();
			CallBack(this);
		}
		
	private:
		virtual void DoWorkInner() = 0;

		TaskGraphNode* NodeOwner {};
		TFunction<void(TaskGraphTask*)> CallBack {};
		NameHandle DebugName;
	};

    struct TaskGraphNode
    {
        TaskGraphNode(TaskGraphTask* InTask)
            : Task(InTask)
        {}
        TaskGraphNode() = delete;
        TaskGraphTask* Task {};
        TArray<TaskGraphTask*> Predecessor {};
        TArray<TaskGraphTask*> Successor {};
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

		void PushTask(TaskGraphTask* Task, const TArray<TaskGraphTask*>& PredecessorList = {});

		void Submit();

		void Reset(); // 等待前序任务完成,重置TG

	private:
		
		void WaitForCompletion() const; // 等待任务完成,且线程退出
	
	private:
		ThreadPoolBase* ThreadPool {};
		TArray<TaskGraphNode*> TaskNodeList {};
		//THashMap<uint32, TaskGraphNode*> NodeDependencyMap {};

		// cv
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<uint32> TaskCount = 0;
	};



    
}
