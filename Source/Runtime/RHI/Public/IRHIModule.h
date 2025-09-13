#pragma once
#include "Module/ModuleManager.h"
#include "RHIContext.h"
#include "RHIResource.h"

namespace Thunder
{
	class IDynamicRHI;
	class RHI_API IRHIModule : public IModule
	{
		friend class ModuleManager;

	public:
		IRHIModule(NameHandle name) : IModule(name) {}
		virtual ~IRHIModule() = default;
		void SetCommandContext(const RHICommandContextRef context)
		{
			RHICommandList = context;
		}
		RHICommandContextRef GetCommandContext() const { return RHICommandList; }
		
		static IRHIModule* GetModule()
		{
			return ModuleInstance;
		}
		
	protected:
		IDynamicRHI* DynamicRHI;
		RHICommandContextRef RHICommandList;
		static IRHIModule* ModuleInstance;
	};
}
