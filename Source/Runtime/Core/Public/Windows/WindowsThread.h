#pragma once
#include "HAL/Event.h"
#include "HAL/Thread.h"

namespace Thunder
{
/**
 * Implements the Windows version of the IEvent interface.
 */
class EventWindows : public IEvent
{
public:
	EventWindows() : IEvent(), Event(nullptr), ManualReset(false) {}

	virtual ~EventWindows() override
	{
		if (Event != nullptr)
		{
			CloseHandle(Event);
		}
	}

	virtual bool Create( bool bIsManualReset = false ) override
	{
		// Create the event and default it to non-signaled
		Event = CreateEvent(nullptr, bIsManualReset, 0, nullptr);
		ManualReset = bIsManualReset;

		return Event != nullptr;
	}

	virtual bool IsManualReset() override
	{
		return ManualReset;
	}

	virtual void Trigger() override
	{
		TAssert( Event );
		SetEvent( Event );
	}
	virtual void Reset() override
	{
		TAssert( Event );
		ResetEvent( Event );
	}
	virtual bool Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats = false ) override;

	virtual void* GetNativeHandle() override
	{
		return static_cast<void*>(Event);
	}

private:
	/** Holds the handle to the event. */
	HANDLE Event;

	/** Whether the signaled state of the event needs to be reset manually. */
	bool ManualReset;
};
	

class ThreadWindows : public IThread
{
	// thread handle
	HANDLE Thread = 0;

	// 创建线程后回调函数入口
	static ::DWORD __stdcall ThreadProc(LPVOID pThis)
	{
		TAssert(pThis);
		auto* ThisThread = static_cast<ThreadWindows*>(pThis);
		return ThisThread->Run();
	}

	uint32 Run();

public:
	~ThreadWindows() override
	{
		if (Thread)
		{
			// 子类析构函数调用虚函数，很不推荐，一定要用的话，需要注意不能访问子类成员，子类此时可能已经析构了
			ThreadWindows::Kill(true);
		}
	}

	virtual void Suspend(bool bShouldPause = true) override
	{
		TAssert(Thread);
		if (bShouldPause == true)
		{
			SuspendThread(Thread);
		}
		else
		{
			ResumeThread(Thread);
		}
	}

	virtual bool Kill(bool bShouldWait = true) override;

	virtual void WaitForCompletion() override
	{
		WaitForSingleObject(Thread, INFINITE);
	}

protected:
	virtual bool CreateInternal(ThreadProxy* InProxy, uint32 InStackSize = 0, const String& InThreadName = "") override;
};

}
