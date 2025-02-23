#pragma once
#include "NameHandle.h"
#include "Task.h"

namespace Thunder
{
	class TaskGraphTask : public ITask
	{
	public:
		TaskGraphTask(const String& InDebugName = "")
			: DebugName(InDebugName) {}

		_NODISCARD_ NameHandle GetName() const
		{
			return DebugName;
		}

		_NODISCARD_ struct TaskGraphNode* GetOwner() const { return NodeOwner; }

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
		TaskGraphProxy(class PooledTaskScheduler* InThreadPool)
			: PooledThread(InThreadPool) {}

		~TaskGraphProxy() = default;

		void PushTask(TaskGraphTask* Task, const TArray<TaskGraphTask*>& PredecessorList = {});

		void Submit();

		void WaitAndReset(); // 等待前序任务完成,重置TG

		void TriggerNextWork(ITask* Task) const;

		void TryNotify();

		void DebugSpot() const;
	
	private:
		PooledTaskScheduler* PooledThread {};
		TArray<TaskGraphNode*> TaskNodeList {};

		// cv
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<uint32> TaskCount = 0;
	};

}
