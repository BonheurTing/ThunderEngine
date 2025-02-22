#pragma once
#include <atomic>
#include "Assertion.h"

namespace Thunder
{
	class IOnCompleted
	{
	public:
		virtual ~IOnCompleted() = default;

		void Promise(int Num)
		{
#if _TRELEASE
			Counter.store(Num, std::memory_order_release);
#else
			TAssert(Counter.exchange(Num) == 0);
#endif
		}

		void Notify()
		{
			if (Counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				OnCompleted();
			}
		}

		virtual void OnCompleted() = 0;

	private:
		std::atomic<int> Counter{ 0 };
	};

	class TaskDispatcher : public IOnCompleted
	{
	public:
		TaskDispatcher(class IEvent* inEvent)
			: Event(inEvent) {}

		void OnCompleted() override;

	private:
		IEvent* Event{};
	};
}
