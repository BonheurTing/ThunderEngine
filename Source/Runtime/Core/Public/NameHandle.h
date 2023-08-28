#pragma once
#include "Platform.h"

namespace Thunder
{
	class CORE_API NameHandle
	{
	public:
		NameHandle(const char* name = nullptr);
		NameHandle(const String& name); 
		NameHandle& operator =(const char* name);
		NameHandle& operator =(const String& name);
		bool operator ==(const NameHandle &rhs) const;
		bool operator ==(const char* rhs) const;
		bool operator ==(const String& rhs) const;
		bool operator !=(const NameHandle &rhs) const;
		bool operator !=(const char* rhs) const;
		bool operator !=(const String& rhs) const;
		bool operator <(const NameHandle &rhs) const;

		[[nodiscard]] bool IsEmpty() const { return StringAddress == Empty.StringAddress; }
		[[nodiscard]] const char* c_str() const { return StringAddress; }

		const char* operator*() const { return StringAddress; }

	public:
		static NameHandle Empty;

	protected:
		friend std::hash<NameHandle>;
	
		const char* StringAddress = nullptr;
	};

	
}

template <>
struct std::hash<Thunder::NameHandle>
{
	size_t operator()(const Thunder::NameHandle& x) const noexcept
	{
		return reinterpret_cast<size_t>(x.StringAddress);
	}
};
