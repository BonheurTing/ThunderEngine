#include "IDynamicRHI.h"

namespace Thunder
{
	IDynamicRHI* GDynamicRHI = NULL;
    
    RHIDeviceRef IDynamicRHI::RHICreateDevice()
    {
    	return nullptr;
    }
}
