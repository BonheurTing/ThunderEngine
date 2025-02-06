#include "Misc/TaskGraph.h"
#include <string>

namespace Thunder
{
    void TaskGraphManager::PushTask(TaskAllocator* Task, std::vector<uint32> PrepositionList)
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

    uint32 TaskGraphManager::CreateWorkerThread()
    {
        uint32 ThreadID;
        Thread = CreateThread(nullptr, 0, ThreadProc, this, 0
        , reinterpret_cast<::DWORD*>(&ThreadID)); //STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED
        if (Thread == nullptr)
        {
            TAssert(false);
        }
        else
        {
            //LOG("ThreadID: %d", ThreadID);
            auto ret = ResumeThread(Thread);
            //LOG("ResumeThread %d: %d", ret, GetLastError());
        }
        return Thread != nullptr;
    }

    uint32 TaskGraphManager::ExecuteImpl()
    {
        //LOG("ExacuteImpl");
        while (auto Task = FindWork())
        {
            Task->DoWork();
            TaskDependencyMap[Task->UniqueId]->bIsComplete = true;
        }
        PrintTaskGraph();

        for (auto& Task : TaskList)
        {
            TAssert(TaskDependencyMap[Task->UniqueId]->bIsComplete);
            delete Task;
        }
        return TaskDependencyMap.size();
    }

    TaskAllocator* TaskGraphManager::FindWork()
    {
        for (auto& Task : TaskList)
        {
            if (TaskDependencyMap[Task->UniqueId]->bIsComplete)
                continue;
				
            bool bHasPreposition = false;
            for (auto Pre : TaskDependencyMap[Task->UniqueId]->Preposition)
            {
                if (!TaskDependencyMap[Pre]->bIsComplete)
                {
                    bHasPreposition = true;
                    break;
                }
            }
            if (!bHasPreposition)
            {
                LOG("FindWork: %s", Task->GetName().c_str());
                return Task;
            }
        }
        return nullptr;
    }

    void TaskGraphManager::PrintTaskGraph()
    {
        TAssert(TaskList.size() == TaskDependencyMap.size());
        String report = "Task Graph has " + std::to_string(TaskList.size()) + " Tasks.";
        for (auto pair : TaskDependencyMap)
        {
            report = report + " name: " + pair.second->Task->GetName().ToString() + ", bIsComplete: " + (pair.second->bIsComplete ? "1" : "0");
        }
    }
    
}