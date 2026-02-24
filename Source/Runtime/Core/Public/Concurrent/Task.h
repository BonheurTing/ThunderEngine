#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
	class ITask
	{
	public:
		CORE_API virtual void DoWork() = 0;
		CORE_API virtual void Abandon() {}
		CORE_API NameHandle GetName() const { return DebugName; }

		CORE_API virtual ~ITask() = default;
	private:
		NameHandle DebugName = "UnKnown";
	};

	// task template: any type which implement DoWork() can use "TTask<TaskType>" to creat task (like functor)
	template<typename TaskType>
	class TTask : public ITask
	{
		TaskType Task;

	public:
		template <typename... ArgTypes>
		TTask(ArgTypes&&... Args)
			: Task(std::forward<ArgTypes>(Args)...)
		{}

		TaskType* operator->() { return &Task; }

		void DoWork() override { Task.DoWork(); }
		void Abandon() override {}
	};


}
