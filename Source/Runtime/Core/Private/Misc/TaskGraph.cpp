#pragma optimize("", off)
#include "Misc/TaskGraph.h"
#include <string>

namespace Thunder
{
    void TaskGraphProxy::PushTask(TGTaskNode* Task, std::vector<uint32> PrepositionList)
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

    void TaskGraphProxy::Submit()
    {
        ThreadPool->SetCallBack([&](ITask* CompletedTask)
        {
            if(auto LocalTask = static_cast<TGTaskNode*>(CompletedTask))
            {
                TAssert(TaskDependencyMap[LocalTask->UniqueId]->State == ETaskState::Ready);
                TaskDependencyMap[LocalTask->UniqueId]->State = ETaskState::Completed;

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
                        TaskDependencyMap[id]->State = ETaskState::Ready;
                        ThreadPool->AddQueuedWork(TaskDependencyMap[id]->Task);
                    }
                }
            }
        });
        
        while (const auto Task = FindWork())
        {
            TAssert(TaskDependencyMap[Task->UniqueId]->State == ETaskState::Wait);
            TaskDependencyMap[Task->UniqueId]->State = ETaskState::Ready;
            TAssert(TaskDependencyMap[Task->UniqueId]->State == ETaskState::Ready);
            
            ThreadPool->AddQueuedWork(Task);
        }
    }

    TGTaskNode* TaskGraphProxy::FindWork()
    {
        for (auto& Task : TaskList)
        {
            if (TaskDependencyMap[Task->UniqueId]->State != ETaskState::Wait)
                continue;
				
            bool bHasPreposition = false;
            for (auto Pre : TaskDependencyMap[Task->UniqueId]->Preposition)
            {
                if (TaskDependencyMap[Pre]->State != ETaskState::Completed)
                {
                    bHasPreposition = true;
                    break;
                }
            }
            if (!bHasPreposition)
            {
                LOG("FindWork: %s", Task->GetName().c_str());
                //PrintTaskGraph();
                return Task;
            }
        }
        return nullptr;
    }

    void TaskGraphProxy::PrintTaskGraph() const
    {
        TAssert(TaskList.size() == TaskDependencyMap.size());
        String report = "Task Graph has " + std::to_string(TaskList.size()) + " Tasks.";
        for (auto pair : TaskDependencyMap)
        {
            report = report + " UniqueId: " + std::to_string(pair.second->Task->UniqueId) + ", State: " + (pair.second->State == ETaskState::Wait ? "Wait" : (pair.second->State == ETaskState::Ready ?  "Ready" : "Completed" ));
        }
        LOG(report.c_str());
    }
    
}
#pragma optimize("", on)