#pragma once
#include "Platform.h"



class CORE_API NameHandle
{
public:
	NameHandle(const char* name = nullptr);

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

protected:
	NameHandle(const String& name); 

public:
	static NameHandle Empty;

protected:
	friend std::hash<NameHandle>;
	
	const char* StringAddress = nullptr;
};

template <>
struct std::hash<NameHandle>
{
	size_t operator()(const NameHandle& x) const noexcept
	{
		return reinterpret_cast<size_t>(x.StringAddress);
	}
};


