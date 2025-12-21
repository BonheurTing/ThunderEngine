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
		void ResetCopyCommandContext_Render(uint32 index) const;
		void ResetCopyCommandContext_RHI(uint32 index) const;

		RHICommandContext* GetCopyCommandContext_Render() const { return CopyCommandContext_Render; }
		RHICommandContext* GetCopyCommandContext_RHI() const { return CopyCommandContext_RHI; }
		const TArray<RHICommandContextRef>& GetRHICommandContexts() const { return CommandContexts; }

		static IRHIModule* GetModule()
		{
			return ModuleInstance;
		}

	protected:
		IDynamicRHI* DynamicRHI;
		TArray<RHICommandContextRef> CommandContexts;
		RHICommandContextRef CopyCommandContext_Render;
		RHICommandContextRef CopyCommandContext_RHI;
		static IRHIModule* ModuleInstance;
	};
}
