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
        Table->Preposition = PrepositionList;
        TaskDependencyMap[Task->UniqueId] = Table;
			
        for (auto Preposition : PrepositionList)
        {
            TaskDependencyMap[Preposition]->Postposition.push_back(Task->UniqueId);
        }
    }

    void TaskGraphProxy::Submit()
    {
        TaskCount.store(TaskDependencyMap.size(), std::memory_order_relaxed);
        
        ThreadPool->SetCallBack([&](ITask* CompletedTask)
        {
            if(const auto LocalTask = static_cast<TGTaskNode*>(CompletedTask))
            {
                TAssert(TaskDependencyMap[LocalTask->UniqueId]->State == ETaskState::Issued);
                TaskDependencyMap[LocalTask->UniqueId]->State = ETaskState::Completed;
                TaskCount.fetch_sub(1, std::memory_order_relaxed);

                for (auto id : TaskDependencyMap[LocalTask->UniqueId]->Postposition)
                {
                    bool bReady = true;
                    for(auto pre : TaskDependencyMap[id]->Preposition)
                    {
                        if (TaskDependencyMap[pre]->State != ETaskState::Completed)
                        {
                            bReady = false;
                            break;
                        }
                    }
                    if (bReady)
                    {
                        TaskDependencyMap[id]->State = ETaskState::Issued;
                        ThreadPool->AddQueuedWork(TaskDependencyMap[id]->Task);
                    }
                }

                if (TaskCount == 0)
                {
                    for(const auto pair : TaskDependencyMap)
                    {
                        TAssert(pair.second->State == ETaskState::Completed);
                    }
                    cv.notify_all();
                }
            }
        });

        for (const auto Task : FindWork())
        {
            TAssert(TaskDependencyMap[Task->UniqueId]->State == ETaskState::Wait);
            TaskDependencyMap[Task->UniqueId]->State = ETaskState::Issued;
            
            ThreadPool->AddQueuedWork(Task);
        }
    }

    void TaskGraphProxy::Reset()
    {
        if (TaskCount.load(std::memory_order_relaxed) > 0)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]{ return TaskCount.load(std::memory_order_relaxed) == 0; });
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

    std::list<TGTaskNode*> TaskGraphProxy::FindWork()
    {
        std::list<TGTaskNode*> ReadyWorks {};
        for (const auto Task : TaskList)
        {
            if (TaskDependencyMap[Task->UniqueId]->Preposition.empty())
            {
                TAssert(TaskDependencyMap[Task->UniqueId]->State == ETaskState::Wait);
                LOG("FindWork: %s", Task->GetName().c_str());
                ReadyWorks.push_back(Task);
            }
        }
        return ReadyWorks;
    }

    void TaskGraphProxy::PrintTaskGraph() const
    {
        TAssert(TaskList.size() == TaskDependencyMap.size());
        String report = "Task Graph has " + std::to_string(TaskList.size()) + " Tasks.";
        for (auto pair : TaskDependencyMap)
        {
            report = report + " UniqueId: " + std::to_string(pair.second->Task->UniqueId) + ", State: " + (pair.second->State == ETaskState::Wait ? "Wait" : (pair.second->State == ETaskState::Issued ?  "Ready" : "Completed" ));
        }
        LOG(report.c_str());
    }
    
}
#pragma optimize("", on)