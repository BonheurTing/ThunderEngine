#pragma once
#include "Module/ModuleManager.h"
#include "RHIContext.h"
#include "RHIResource.h"

namespace Thunder
{
	class IDynamicRHI;
	class IRHIModule : public IModule
	{
		friend class ModuleManager;

	public:
		RHI_API IRHIModule(NameHandle name) : IModule(name) {}
		RHI_API virtual ~IRHIModule() = default;

		RHI_API void InitCommandContext();
		RHI_API void ResetCommandContext(uint32 index) const;
		RHI_API void ResetCopyCommandContext_Render(uint32 index) const;
		RHI_API void ResetCopyCommandContext_RHI(uint32 index) const;

		RHI_API RHICommandContext* GetCopyCommandContext_Render() const { return CopyCommandContext_Render; }
		RHI_API RHICommandContext* GetCopyCommandContext_RHI() const { return CopyCommandContext_RHI; }
		RHI_API const TArray<RHICommandContextRef>& GetRHICommandContexts() const { return CommandContexts; }
		RHI_API RHICommandContextRef GetRHICommandContext(uint32 index) const { return CommandContexts[index]; }

		RHI_API static IRHIModule* GetModule()
		{
			return ModuleInstance;
		}

	protected:
		IDynamicRHI* DynamicRHI;
		TArray<RHICommandContextRef> CommandContexts;
		RHICommandContextRef CopyCommandContext_Render;
		RHICommandContextRef CopyCommandContext_RHI;
		RHI_API static IRHIModule* ModuleInstance;
	};
}
