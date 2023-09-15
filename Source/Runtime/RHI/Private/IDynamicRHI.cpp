#include "IDynamicRHI.h"

namespace Thunder
{
	IDynamicRHI* GDynamicRHI = nullptr;
    
    RHIDeviceRef IDynamicRHI::RHICreateDevice()
    {
    	return nullptr;
    }
}
