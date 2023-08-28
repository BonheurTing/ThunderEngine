#include "DynamicRHI.h"

namespace Thunder
{
	DynamicRHI* GDynamicRHI = NULL;
    
    RHIDeviceRef DynamicRHI::RHICreateDevice()
    {
    	return nullptr;
    }
}
