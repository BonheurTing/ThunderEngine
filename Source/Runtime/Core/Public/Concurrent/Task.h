#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
	class CORE_API ITask
	{
	public:
		virtual void DoWork() = 0;
		virtual void Abandon() {}
		NameHandle GetName() const { return DebugName; }

		virtual ~ITask() = default;
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

	public:
		void DoWork() override { Task.DoWork(); }
		void Abandon() override {}
	};


}
