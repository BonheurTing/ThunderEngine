#pragma once
#include "IRHIModule.h"

struct ID3D12Device;
namespace Thunder
{
	class TD3D12PipelineStateCache;
	
	class D3D12RHI_API TD3D12RHIModule : public IRHIModule
	{
		DECLARE_MODULE_WITH_SUPER(D3D12RHI, TD3D12RHIModule, IRHIModule)
	public:
		void StartUp() override;
		void ShutDown() override {}

		void InitPipelineStateTable(ID3D12Device* InDevice);
		_NODISCARD_ TD3D12PipelineStateCache* GetPipelineStateTable() const { return PipelineStateTable.get(); }
	private:
		RefCountPtr<TD3D12PipelineStateCache> PipelineStateTable;
	};
}
