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
			TAssertf(Num > 0, "Promise called with invalid num: %d", Num);
			if (Num <= 0)
			{
				OnCompleted();
				return;
			}

			const int prev = Counter.fetch_add(Num, std::memory_order_acq_rel);
			TAssertf(prev <= 0, "Promise called twice or Counter corrupted: prev=%d", prev);
			if (prev + Num == 0)
			{
				OnCompleted();
			}
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
