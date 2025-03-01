#pragma once
#include "Container.h"


namespace Thunder
{
	class CORE_API ITask
	{
	public:
		virtual void DoWork() = 0;
		virtual void Abandon() {}
	public:
		virtual ~ITask() = default;
	};

	// 任务模板：任意实现了DoWork函数的类型，可用TTask<TaskType>创建任务(有点像仿函数的原理)
	template<typename TaskType>
	class TTask : public ITask
	{
		// 内嵌任务类型
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
