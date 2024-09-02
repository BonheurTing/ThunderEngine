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

	/**
	 * Gets an event from the pool or creates one if necessary.
	 *
	 * @return The event.
	 * @see ReturnToPool
	 */
	IEvent* GetEventFromPool()
	{
		return new SafeRecyclableEvent(GetRawEvent());
	}

	/**
	 * Returns an event to the pool.
	 *
	 * @param Event The event to return.
	 * @see GetEventFromPool
	 */
	void ReturnToPool(IEvent* Event)
	{
		TAssert(Event);
		TAssert(Event->IsManualReset() == (PoolType == EEventMode::ManualReset));

		SafeRecyclableEvent* SafeEvent = (SafeRecyclableEvent*)Event;
		ReturnRawEvent(SafeEvent->InnerEvent);
		delete SafeEvent;
	}

	void EmptyPool()
	{
		while (IEvent* Event = Pool.Pop())
		{
			delete Event;
		}
	}

	/**
	* Gets a "raw" event (as opposite to `FSafeRecyclableEvent` handle returned by `GetEventFromPool`) from the pool or 
	* creates one if necessary.
	* @see ReturnRaw
	*/
	IEvent* GetRawEvent()
	{
		IEvent* Event = Pool.Pop();

		if (Event == nullptr)
		{
			Event = FPlatformProcess::GetSyncEventFromPool(PoolType == EEventMode::ManualReset);
		}

		TAssert(Event);

		return Event;
	}

	/**
	 * Returns a "raw" event to the pool.
	 * @see GetRaw
	 */
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
