#pragma once

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
	using Array = std::vector<InElementType, InAllocatorType>;

	template<typename InElementType>
	using HashSet = std::unordered_set<InElementType>;

	template<typename InElementType>
	using Set = std::set<InElementType>;

	template<typename InKeyType, typename InValueType>
	using HashMap = std::unordered_map<InKeyType, InValueType>;

	template<typename InKeyType, typename InValueType>
	using Map = std::map<InKeyType, InValueType>;

	template<typename InElementType>
	using Deque = std::deque<InElementType>;

	using Mutex = std::mutex;
	using ScopeLock = std::lock_guard<std::mutex>;

	////// Function type macros.
	#define FORCEINLINE __forceinline									/* Force code to be inline */

#define LOG(...) printf(__VA_ARGS__); \
	std::cout << std::endl
	
}