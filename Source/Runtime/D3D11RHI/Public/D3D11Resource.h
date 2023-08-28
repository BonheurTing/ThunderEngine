#pragma once

#include "D3D11.h"
#include "RHIResource.h"

namespace Thunder
{
	class D3D11Device : public RHIDevice
    {
    public:
    	D3D11Device() = delete;
    	~D3D11Device() = default;
    	D3D11Device(ID3D11Device * InDevice): mDevice(InDevice) {}
    
    private:
    	ID3D11Device * mDevice;
    };
}
