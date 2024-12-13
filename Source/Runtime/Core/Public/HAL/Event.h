#pragma once
#include "Assertion.h"
#include "PlatformProcess.h"
#include "Container/LockFree.h"

namespace Thunder
{

class CORE_API IEvent
{
public:
	virtual bool Create( bool bIsManualReset = false ) = 0;
	virtual bool IsManualReset() = 0;
	virtual void Trigger() = 0;
	virtual void Reset() = 0;
	virtual bool Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats = false ) = 0;
	bool Wait() { return Wait((uint32)0xffffffff); }
public:
	IEvent() = default;
	virtual ~IEvent() = default;
	IEvent(IEvent const&) = default;
	IEvent& operator =(IEvent const&) = default;
	IEvent(IEvent&&) = default;
	IEvent& operator=(IEvent&&) = default;
private:
	uint32 EventId;
};

class FScopedEvent
{
public:

	/** Default constructor. */
	CORE_API FScopedEvent();

	/** Destructor. */
	CORE_API ~FScopedEvent();

	/** Triggers the event. */
	void Trigger()
	{
		Event->Trigger();
	}

	/**
	 * Checks if the event has been triggered (used for special early out cases of scope event)
	 * if this returns true once it will return true forever
	 *
	 * @return returns true if the scoped event has been triggered once
	 */
	CORE_API bool IsReady();

	/**
	 * Retrieve the event, usually for passing around.
	 *
	 * @return The event.
	 */
	IEvent* Get()
	{
		return Event;
	}

private:

	/** Holds the event. */
	IEvent* Event;
};

enum class EEventMode : uint8 { AutoReset, ManualReset };

// 给IEvent包了一层，析构的时候释放
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

};

/**
 * 模板事件池
 * 提供一种高效地回收和重用事件对象的方法，以减少事件对象的创建和销毁开销。通过使用事件对象池，可以避免频繁地创建和销毁事件对象，提高性能和效率
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
