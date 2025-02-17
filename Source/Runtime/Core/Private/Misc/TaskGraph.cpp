#pragma optimize("", off)
#include "Misc/TaskGraph.h"

#include <ranges>

#include "Misc/TheadPool.h"

namespace Thunder
{
    void TaskGraphProxy::PushTask(TGTaskNode* Task, std::vector<uint32> PrepositionList)
    {
        TAssert(!TaskDependencyMap.contains(Task->UniqueId));
        TaskList.push_back(Task);
        const auto Table = new FTaskTable(Task);
        Table->Predecessor = PrepositionList;
        Table->PredecessorNum.store(PrepositionList.size(), std::memory_order_relaxed);
        TaskDependencyMap[Task->UniqueId] = Table;
			
        for (auto Preposition : PrepositionList)
        {
            TaskDependencyMap[Preposition]->Successor.push_back(Task->UniqueId);
        }
    }

    void TaskGraphProxy::Submit()
    {
        TaskCount.store(TaskDependencyMap.size(), std::memory_order_relaxed);
        
        ThreadPool->SetCallBack([&](ITask* CompletedTask)
        {
            if(const auto LocalTask = static_cast<TGTaskNode*>(CompletedTask))
            {
                TaskCount.fetch_sub(1, std::memory_order_release);

                for (auto id : TaskDependencyMap[LocalTask->UniqueId]->Successor)
                {
                    if (TaskDependencyMap[id]->PredecessorNum.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    {
                        ThreadPool->AddQueuedWork(TaskDependencyMap[id]->Task);
                    }
                }

                if (TaskCount.load(std::memory_order_acquire) == 0)
                {
                    for(const auto val : TaskDependencyMap | std::views::values)
                    {
                        TAssert(val->PredecessorNum.load(std::memory_order_acquire) == 0);
                    }
                    cv.notify_all();
                }
            }
        });

        for (const auto val : TaskDependencyMap | std::views::values)
        {
            if (val->PredecessorNum.load(std::memory_order_acquire) == 0)
            {
                ThreadPool->AddQueuedWork(val->Task);
            }
        }
    }

    void TaskGraphProxy::Reset()
    {
        if (TaskCount.load(std::memory_order_acquire) > 0)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]{ return TaskCount.load(std::memory_order_acquire) == 0; });
        }
        for (const auto val : TaskDependencyMap | std::views::values)
        {
            delete val->Task;
            delete val;
        }
        TaskList.clear();
        TaskDependencyMap.clear();
    }

    void TaskGraphProxy::WaitForCompletion() const
    {
        ThreadPool->WaitForCompletion();
    }
    
}
#pragma optimize("", on)