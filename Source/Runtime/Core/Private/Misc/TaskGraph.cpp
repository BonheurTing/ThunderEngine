#pragma optimize("", off)
#include "Misc/TaskGraph.h"

#include <ranges>

#include "Misc/TheadPool.h"

namespace Thunder
{
    void TaskGraphProxy::PushTask(TaskGraphTask* Task, const TArray<TaskGraphTask*>& PredecessorList)
    {
        const auto NewNode = new TaskGraphNode(Task);
        NewNode->Predecessor = PredecessorList;
        NewNode->PredecessorNum.store(static_cast<uint32>(PredecessorList.size()), std::memory_order_release);
        Task->SetOwner(NewNode);
        Task->SetCallBack([&](TaskGraphTask* CompletedTask)
        {
            if (const auto CompletedNode = CompletedTask->GetOwner())
            {
                for (const auto SuccessorTask : CompletedNode->Successor)
                {
                    if (const auto SucNode = SuccessorTask->GetOwner())
                    {
                        if(SucNode->PredecessorNum.fetch_sub(1, std::memory_order_acq_rel) == 1)
                        {
                            ThreadPool->AddQueuedWork(SuccessorTask);
                        }
                    }
                }

                if (TaskCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    cv.notify_all();
                }
            }
        });

        for (const auto PredecessorTask : PredecessorList)
        {
            if (const auto Node = PredecessorTask->GetOwner())
            {
                Node->Successor.push_back(Task);
            }
        }
        TaskNodeList.push_back(NewNode);
    }

    void TaskGraphProxy::Submit()
    {
        TaskCount.store(static_cast<uint32>(TaskNodeList.size()), std::memory_order_release);

        for (const auto Node : TaskNodeList)
        {
            if (Node->Predecessor.empty())
            {
                ThreadPool->AddQueuedWork(Node->Task);
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
        for (auto Node : TaskNodeList)
        {
            TAssert(Node->PredecessorNum.load(std::memory_order_acquire) == 0);
            delete Node->Task;
            Node->Task = nullptr;
            delete Node;
            Node = nullptr;
        }
        TaskNodeList.clear();
    }

    void TaskGraphProxy::WaitForCompletion() const
    {
        ThreadPool->WaitForCompletion();
    }
    
}
#pragma optimize("", on)