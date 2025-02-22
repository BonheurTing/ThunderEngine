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

		_NODISCARD_ NameHandle GetName() const
		{
			return DebugName;
		}

		_NODISCARD_ TaskGraphNode* GetOwner() const { return NodeOwner; }

		void SetOwner(TaskGraphNode* InNode) { NodeOwner = InNode; }

		virtual void DoWork() final
		{
			DoWorkInner();
			OnCompleted(this);
		}
		
	private:
		virtual void DoWorkInner() = 0;
		static void OnCompleted(const TaskGraphTask* CompletedTask);

		TaskGraphNode* NodeOwner {};
		NameHandle DebugName;
	};

    struct TaskGraphNode
    {
        TaskGraphNode(class TaskGraphProxy* InOwner, TaskGraphTask* InTask)
            : TaskGraphOwner(InOwner), Task(InTask)
        {}

    	~TaskGraphNode()
        {
        	TMemory::Free(Task);
        }

        TaskGraphNode() = delete;
    	TaskGraphProxy* TaskGraphOwner {};
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

		void TriggerNextWork(ITask* Task) const;

		void TryNotify();

	private:
		
		void WaitForCompletion() const; // 等待任务完成,且线程退出
	
	private:
		ThreadPoolBase* ThreadPool {};
		TArray<TaskGraphNode*> TaskNodeList {};

		// cv
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<uint32> TaskCount = 0;
	};

}
