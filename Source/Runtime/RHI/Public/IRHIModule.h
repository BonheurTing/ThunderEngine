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
		
		bool TryGetCopyCommandContext() const { return CommandContexts.size() > 1 && CommandContexts.back().IsValid(); }
		RHICommandContext* GetCopyCommandContext() const { return CommandContexts.size() > 1 ? CommandContexts.back().Get() : nullptr; }
		const TArray<RHICommandContextRef>& GetRHICommandContexts() const { return CommandContexts; }

		static IRHIModule* GetModule()
		{
			return ModuleInstance;
		}

	protected:
		IDynamicRHI* DynamicRHI;
		TArray<RHICommandContextRef> CommandContexts;
		static IRHIModule* ModuleInstance;
	};
}
