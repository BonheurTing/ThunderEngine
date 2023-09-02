#pragma once
#include <memory>
#include "BasicDefinition.h"

namespace Thunder
{
	template<typename ReferencedType>
	using RefCountPtr = std::shared_ptr<ReferencedType>;
	
	template <class Type, class... ArgTypes>
	_NODISCARD_ auto MakeRefCount(ArgTypes&&... args)
	{
		RefCountPtr<Type> ret = std::make_shared<Type>(std::forward<ArgTypes>(args)...);
		return ret;
	}
}
