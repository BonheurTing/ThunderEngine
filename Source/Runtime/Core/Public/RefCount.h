#pragma once
#include <memory>

namespace Thunder
{
	template<typename ReferencedType>
	using RefCountPtr = std::shared_ptr<ReferencedType>;
}
