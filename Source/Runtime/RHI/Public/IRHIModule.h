#pragma once
#include "Module/ModuleManager.h"

namespace Thunder
{
	class IDynamicRHI;
	class RHI_API IRHIModule : public IModule
	{
	public:
		IRHIModule(NameHandle name) : IModule(name) {}
		virtual ~IRHIModule() = default;
		virtual void CreatRHI() = 0;
	protected:
		IDynamicRHI* DynamicRHI = nullptr;
	};
}
