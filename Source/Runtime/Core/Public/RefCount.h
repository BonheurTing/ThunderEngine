#pragma once
#include <memory>
#include "BasicDefinition.h"

namespace Thunder
{
	template<typename ReferencedType>
	using RefCountPtrL = std::shared_ptr<ReferencedType>;
	
	template <typename Type, typename... ArgTypes>
	_NODISCARD_ auto MakeRefCountL(ArgTypes&&... args)
	{
		RefCountPtrL<Type> ret = std::make_shared<Type>(std::forward<ArgTypes>(args)...);
		return ret;
	}
}
