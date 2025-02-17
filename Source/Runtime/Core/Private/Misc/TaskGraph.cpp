#pragma optimize("", off)
#include "Misc/TaskGraph.h"

#include <ranges>

#include "Misc/TheadPool.h"

namespace Thunder
{
    void TaskGraphProxy::PushTask(TGTaskNode* Task, const TArray<uint32>& PrepositionList)
    {
        TAssert(!TaskDependencyMap.contains(Task->UniqueId));
        TaskList.push_back(Task);
        const auto Table = new FTaskTable(Task);
        Table->Predecessor = PrepositionList;
        TArray<uint32> d = {};
        TAssert(d.size()==0);
        TAssert(PrepositionList.size()>=0);
        TAssert(PrepositionList.size()<=1);
        Table->PredecessorNum.store(static_cast<uint32>(PrepositionList.size()), std::memory_order_release);
        TaskDependencyMap[Task->UniqueId] = Table;
			
        for (auto Preposition : PrepositionList)
        {
            TAssert(TaskDependencyMap.contains(Preposition));
            TaskDependencyMap[Preposition]->Successor.push_back(Task->UniqueId);
        }
    }

    void TaskGraphProxy::Submit()
    {
        TAssert(TaskList.size() == TaskDependencyMap.size());
        TaskCount.store(static_cast<unsigned>(TaskDependencyMap.size()), std::memory_order_release);
        
        ThreadPool->SetCallBack([&](ITask* CompletedTask)
        {
            if(const auto LocalTask = static_cast<TGTaskNode*>(CompletedTask))
            {
                TAssert(TaskList.size() == TaskDependencyMap.size());
                TAssert(TaskDependencyMap.contains(LocalTask->UniqueId));
                TAssert(TaskDependencyMap[LocalTask->UniqueId]->Task);

                for (auto id : TaskDependencyMap[LocalTask->UniqueId]->Successor)
                {
                    TAssert(TaskList.size() == TaskDependencyMap.size());
                    TAssert(TaskDependencyMap.contains(id));
                    TAssert(TaskDependencyMap[id]->PredecessorNum.load(std::memory_order_acquire) > 0 );
                    if (TaskDependencyMap[id]->PredecessorNum.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    {
                        TAssert(TaskList.size() == TaskDependencyMap.size());
                        ThreadPool->AddQueuedWork(TaskDependencyMap[id]->Task);
                    }
                }

                if (TaskCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    TAssert(TaskList.size() == TaskDependencyMap.size());
                    for(const auto val : TaskDependencyMap | std::views::values)
                    {
                        TAssert(val->Task);
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
        for (auto val : TaskDependencyMap | std::views::values)
        {
            TAssert(TaskList.size() == TaskDependencyMap.size());
            TAssert(val->PredecessorNum.load(std::memory_order_acquire) == 0);
            delete val->Task;
            val->Task = nullptr;
            delete val;
            val = nullptr;
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