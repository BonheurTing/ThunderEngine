#include "NameHandle.h"
#include "Assertion.h"

namespace Thunder
{
	NameHandle NameHandle::Empty(String(""));

	struct NameEntry
	{
		NameEntry*	Next = nullptr;
		uint32		Hash = 0;
		char		StringPtr[1] = { '\0' };
	};

	#define HASH_TABLE_SIZE (101111)

	class NamePool  // NOLINT(cppcoreguidelines-special-member-functions)
	{
	public: 
		NamePool();
		~NamePool();

		static NamePool& Get() 
		{
			static NamePool* instance = nullptr;
			if (!instance)
			{
				instance = new NamePool();
			}
			return *instance;
		}

		const char* GetStringAddress(const char* name);
		const char* FindStringAddress(const char* name);
		const char* FindStringAddress(const char *name, uint32 hash) const;

	protected:
		static uint32 CalculateStringHash(const char* name);
		const char * AddNameToPool(const char*name, uint32 hash);
		void AddStringPool();

	protected:
		Mutex PoolMutex;
		TArray<char*> StringPools;
		size_t PoolSize = 1048576;
		size_t CurrentStringOffset = 0;
		char* CurrentStringPool = nullptr;
		NameEntry *StringList[HASH_TABLE_SIZE] = { nullptr };
	};

	NamePool::NamePool()
	{
		ScopeLock lock(PoolMutex);
		AddStringPool();
	}

	NamePool::~NamePool()
	{
		for (auto& StringPool : StringPools)
		{
			delete StringPool;
			StringPool = nullptr;
		}
	}

	void NamePool::AddStringPool()
	{
		CurrentStringPool = static_cast<char*>(malloc(PoolSize));
		memset(CurrentStringPool, 0, PoolSize);
		CurrentStringOffset = 0;
		StringPools.push_back(CurrentStringPool);
	}

	uint32 NamePool::CalculateStringHash(const char* name)
	{
		const std::string nameString{name};
		constexpr std::hash<std::string> hashFunction;
		const auto stringHash = static_cast<uint32>(hashFunction(nameString));
		return stringHash;
	}

	const char* NamePool::AddNameToPool(const char *name, uint32 hash)
	{
		ScopeLock lock(PoolMutex);

		if (const char* existAddress = FindStringAddress(name, hash))
		{
			return existAddress;
		}
		const uint32 hashIndex = hash % HASH_TABLE_SIZE;
		NameEntry* next = StringList[hashIndex];

		constexpr size_t strOverhead = sizeof(NameEntry::Next) + sizeof(NameEntry::Hash);   // NOLINT(bugprone-sizeof-expression)
		const size_t strLength = strlen(name) + sizeof('\0');
		const size_t strPadding = sizeof(void*) - ((strOverhead + strLength) & (sizeof(void*) - 1ULL));

		if ((CurrentStringOffset + strLength + strOverhead + strPadding) * sizeof(char) > PoolSize)
		{
			AddStringPool();
		}

		auto list = reinterpret_cast<NameEntry*>(CurrentStringOffset + CurrentStringPool);
		list->Hash = CalculateStringHash(name);

	#ifdef UNICODE
		memcpy(list->StringPtr, name, strLength * 2);
	#else
		memcpy(list->mString, name, strLength);
	#endif
		TAssert(list->StringPtr[strLength - 1] == 0);
		CurrentStringOffset += strLength + strOverhead + strPadding;
		TAssert(CurrentStringOffset % sizeof(void*) == 0);
		StringList[hashIndex] = list;
		StringList[hashIndex]->Next = next;

		return list->StringPtr;
	}

	const char* NamePool::FindStringAddress(const char *name, uint32 hash) const
	{
		NameEntry* list = StringList[hash % HASH_TABLE_SIZE];
		while (list)
		{
			if (list->Hash == hash && strcmp(name, list->StringPtr) == 0)
			{
				return list->StringPtr;
			}
			list = list->Next;
		}
		return nullptr;
	}

	const char* NamePool::FindStringAddress(const char* name)
	{
		const uint32 Hash = CalculateStringHash(name);
		ScopeLock lock(PoolMutex);
		return FindStringAddress(name, Hash);
	}

	const char* NamePool::GetStringAddress(const char* name)
	{
		const uint32 hash = CalculateStringHash(name);
		PoolMutex.lock();
		const char* existAddress = FindStringAddress(name, hash);
		PoolMutex.unlock();
		if (existAddress)
		{
			return existAddress;
		}

		return AddNameToPool(name, hash);
	}

	NameHandle::NameHandle(const char* name)
	{
		if (name == nullptr || *name == '\0')
		{
			StringAddress = Empty.StringAddress;
		}
		else
		{
			StringAddress = NamePool::Get().GetStringAddress(name);
		}
	}

	NameHandle::NameHandle(const String& name)
	{
		StringAddress = NamePool::Get().GetStringAddress(name.c_str());
	}

	NameHandle& NameHandle::operator=(const char* name)
	{
		if (name == nullptr || *name == '\0')
		{
			StringAddress = Empty.StringAddress;
		}
		else if (name == StringAddress)
		{
			return *this;
		}
		else
		{
			StringAddress = NamePool::Get().GetStringAddress(name);
		}

	    return *this;
	}

	NameHandle& NameHandle::operator=(const String& name)
	{
	    return (*this)=(name.c_str());
	}

	bool NameHandle::operator==(const NameHandle &rhs) const
	{
		return StringAddress == rhs.StringAddress;
	}

	bool NameHandle::operator==(const char* rhs) const
	{
		return StringAddress == NameHandle(rhs).StringAddress;
	}

	bool NameHandle::operator==(const String& rhs) const
	{
		return StringAddress == NameHandle(rhs.c_str()).StringAddress;
	}

	bool NameHandle::operator!=(const NameHandle &rhs) const
	{
		return StringAddress != rhs.StringAddress;
	}

	bool NameHandle::operator!=(const char* rhs) const
	{
		return StringAddress != NameHandle(rhs).StringAddress;
	}

	bool NameHandle::operator!=(const String& rhs) const
	{
		return StringAddress != NameHandle(rhs.c_str()).StringAddress;
	}

	bool NameHandle::operator<(const NameHandle &rhs) const
	{
		return StringAddress < rhs.StringAddress;
	}
}