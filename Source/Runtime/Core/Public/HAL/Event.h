#pragma once
#include "Assertion.h"
#include "PlatformProcess.h"
#include "Container/LockFree.h"

namespace Thunder
{

class IEvent
{
public:
	CORE_API virtual bool Create( bool bIsManualReset = false ) = 0;
	CORE_API virtual bool IsManualReset() = 0;
	CORE_API virtual void Trigger() = 0;
	CORE_API virtual void Reset() = 0;
	CORE_API virtual bool Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats = false ) = 0;
	CORE_API bool Wait() { return Wait((uint32)0xffffffff); }
	CORE_API virtual void* GetNativeHandle() = 0;

	CORE_API IEvent() = default;
	CORE_API virtual ~IEvent() = default;
	CORE_API IEvent(IEvent const&) = default;
	CORE_API IEvent& operator =(IEvent const&) = default;
	CORE_API IEvent(IEvent&&) = default;
	CORE_API IEvent& operator=(IEvent&&) = default;
private:
	uint32 EventId;
};

enum class EEventMode : uint8 { AutoReset, ManualReset };

// Release when destruction
class SafeRecyclableEvent  final: public IEvent
{
public:
	IEvent* InnerEvent;
	SafeRecyclableEvent(IEvent* InInnerEvent)
		: InnerEvent(InInnerEvent) {}
	~SafeRecyclableEvent() override
	{
		InnerEvent = nullptr;
	}

	virtual bool Create(bool bIsManualReset = false) override
	{
		return InnerEvent->Create(bIsManualReset);
	}

	virtual bool IsManualReset() override
	{
		return InnerEvent->IsManualReset();
	}

	virtual void Trigger() override
	{
		InnerEvent->Trigger();
	}

	virtual void Reset() override
	{
		InnerEvent->Reset();
	}

	virtual bool Wait(uint32 WaitTime, const bool bIgnoreThreadIdleStats = false) override
	{
		return InnerEvent->Wait(WaitTime, bIgnoreThreadIdleStats);
	}

	virtual void* GetNativeHandle() override
	{
		return InnerEvent->GetNativeHandle();
	}

};

/**
 * thread pool template
 * Provide an efficient method for recycling and reusing event objects to reduce the cost of creating and destroying event objects.
 * By using an event object pool, frequent creation and destruction of event objects can be avoided, thereby improving performance and efficiency.
 */
template<EEventMode PoolType>
class TEventPool
{
public:
	~TEventPool()
	{
		EmptyPool();
	}

	IEvent* GetEventFromPool()
	{
		return new SafeRecyclableEvent(GetRawEvent());
	}

	void ReturnToPool(IEvent* Event)
	{
		TAssert(Event);
		TAssert(Event->IsManualReset() == (PoolType == EEventMode::ManualReset));

		const SafeRecyclableEvent* SafeEvent = static_cast<SafeRecyclableEvent*>(Event);
		ReturnRawEvent(SafeEvent->InnerEvent);
		delete SafeEvent;
	}

	void EmptyPool()
	{
		while (const IEvent* Event = Pool.Pop())
		{
			delete Event;
		}
	}

	IEvent* GetRawEvent()
	{
		IEvent* Event = Pool.Pop();
		if (Event == nullptr)
		{
			Event = FPlatformProcess::CreateSyncEvent(PoolType == EEventMode::ManualReset);
		}
		TAssert(Event);
		return Event;
	}

	void ReturnRawEvent(IEvent* Event)
	{
		TAssert(Event);
		Event->Reset();
		Pool.Push(Event);
	}

private:
	/** Holds the collection of recycled events. */
	TLockFreeListUnordered<IEvent, PLATFORM_CACHE_LINE_SIZE> Pool;
};
	

}
