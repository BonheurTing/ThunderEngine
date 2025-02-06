#pragma once
#include "NameHandle.h"
#include "Misc/FTaskGraphInterface.h"


namespace Thunder
{
    std::atomic<uint32> GTaskUniqueID = 0;

    class TaskAllocator : public Noncopyable
    {
    public:
        TaskAllocator(const String& InDebugName = "")
            : DebugName(InDebugName)
        {
            UniqueId = GTaskUniqueID.fetch_add(1);
        }
        virtual  ~TaskAllocator() {}

        NameHandle GetName() const
        {
            return DebugName;
        }

        virtual void DoWork() = 0;
	
        uint32 UniqueId;

    private:
        NameHandle DebugName;
    };

    struct FTaskTable
    {
        FTaskTable(TaskAllocator* InTask)
            : Task(InTask)
        {}
        FTaskTable() = delete;
        TaskAllocator* Task;
        std::vector<uint32> Preposition;
        std::vector<uint32> Postposition;
        bool bIsComplete = false;
    };

	class TaskGraphManager
	{
	public:
		~TaskGraphManager()
		{
			WaitForComplete();
			CloseHandle(Thread);
		}
		
		void PushTask(TaskAllocator* Task, std::vector<uint32> PrepositionList = {});

		uint32 CreateWorkerThread();

		void WaitForComplete()
		{
			WaitForSingleObject(Thread, INFINITE);
		}
	private:
		static ::DWORD __stdcall ThreadProc(LPVOID pThis)
		{
			//LOG("ThreadProc");
			TAssert(pThis);
			auto* ThisThread = static_cast<TaskGraphManager*>(pThis);
			return ThisThread->ExecuteImpl();
		}
		
		uint32 ExecuteImpl();

		TaskAllocator* FindWork();

		void PrintTaskGraph();

	private:
		HANDLE Thread;
		std::vector<TaskAllocator*> TaskList;
		std::unordered_map<uint32, FTaskTable*> TaskDependencyMap; //
	};



    
}