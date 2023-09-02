#pragma once
#include "IRHIModule.h"

namespace Thunder
{
	class TD3D11RHIModule : public IRHIModule
	{
	public:
		void CreatRHI() override;
	};
}
