#pragma once
#include <memory>

template<typename ReferencedType>
using RefCountPtr = std::shared_ptr<ReferencedType>;
