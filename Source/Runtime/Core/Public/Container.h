#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <deque>
#include <set>
#include <unordered_set>

namespace Thunder
{
	using String = std::string;

	template<typename InElementType, typename InAllocatorType = std::allocator<InElementType>>
	using TArray = std::vector<InElementType, InAllocatorType>;

	template<typename InElementType>
	using TList = std::list<InElementType>;
	
	template<typename InElementType>
	using THashSet = std::unordered_set<InElementType>;

	template<typename InElementType>
	using TSet = std::set<InElementType>;

	template<typename InKeyType, typename InValueType>
	using THashMap = std::unordered_map<InKeyType, InValueType>;

	template<typename InKeyType, typename InValueType>
	using TMap = std::map<InKeyType, InValueType>;

	template<typename InElementType>
	using TDeque = std::deque<InElementType>;

	using Mutex = std::mutex;
	using ScopeLock = std::lock_guard<std::mutex>;

	////// Function type macros.
	#define FORCEINLINE __forceinline									/* Force code to be inline */

#define LOG(...) printf(__VA_ARGS__); \
	std::cout << std::endl
	
}