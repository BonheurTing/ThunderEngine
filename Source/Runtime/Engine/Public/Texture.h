#pragma once
#include "GameObject.h"

namespace Thunder
{
	class ITexture : public GameResource
	{
	public:
		ITexture() = default;
		virtual ~ITexture() = default;
	};
}
