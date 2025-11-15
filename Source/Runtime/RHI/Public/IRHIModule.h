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

		void InitCommandContext();
		void ResetCommandContext(uint32 index) const;
		void ResetCopyCommandContext(uint32 index) const;

		RHICommandContext* GetCopyCommandContext() const { return CopyCommandContext; }
		const TArray<RHICommandContextRef>& GetRHICommandContexts() const { return CommandContexts; }

		static IRHIModule* GetModule()
		{
			return ModuleInstance;
		}

	protected:
		IDynamicRHI* DynamicRHI;
		TArray<RHICommandContextRef> CommandContexts;
		RHICommandContextRef CopyCommandContext;
		static IRHIModule* ModuleInstance;
	};
}
