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

	// 存储TFunction的任务，需要能接任意类型的TFunction，需要存储TFunction的参数类型，如用指针，或者模板template<typename T1, T2...>, 当前写法只能存储TFunction<void()>
	class FTask : public ITask
	{
	public:
		FTask(const TFunction<void()> InFunction)
			: Function(InFunction)
		{}

		void DoWork() override { Function(); }
	private:

		TFunction<void()> Function;
	};

	// 直接继承ITask接口的类型
	class ExampleInheritedTask : public ITask
	{
	public:
		ExampleInheritedTask(int InData)
			: Data(InData)
		{
		}

		void DoWork() override
		{
			LOG("Execute ExampleInheritedTask(Data: %d) with thread: %lu", Data, __threadid());
		}
		void Abandon() override {}
	private:
		int Data;
	};

	// 任务模板：任意实现了DoWork函数的类型，可用TTask<TaskType>创建任务(有点像仿函数的原理)
	template<typename TaskType>
	class TTask : public ITask
	{
		// 内嵌任务类型
		TaskType Task;

	public:
		TTask()
			: Task()
		{
		}

		template <typename Arg0Type, typename... ArgTypes>
		TTask(Arg0Type&& Arg0, ArgTypes&&... Args)
			: Task(std::forward<Arg0Type>(Arg0), std::forward<ArgTypes>(Args)...)
		{
		}

	public:
		void DoWork() override { Task.DoWork(); }
		void Abandon() override {}
	};
/*
	class ExampleTask
	{
	public:
		friend class TTask<ExampleTask>;

		int32 Data;
		ExampleTask(): Data(0) {}

		ExampleTask(int32 InExampleData)
		 : Data(InExampleData) {}

		void DoWork() const
		{
			LOG("ExampleData Add 1: %d", Data + 1);
		}
	};

	inline void TestTaskMain()
	{
		TTask<ExampleTask>* TemplateTask = new TTask<ExampleTask>(1);
		TemplateTask->DoWork();
	}
*/
#pragma endregion 

}
