#include "DynamicRHI.h"

DynamicRHI* GDynamicRHI = NULL;

RHIDeviceRef DynamicRHI::RHICreateDevice()
{
	return nullptr;
}