#pragma optimize("", off)

#include "Assertion.h"
#include "CoreMinimal.h"
#include "CoreModule.h"
#include "FileManager.h"
#include "Templates/RefCounting.h"

using namespace Thunder;


template<typename ReferencedType, bool IsManaged = false>
class TRefCountPtr2
{
	typedef ReferencedType* ReferenceType;

public:
	// constructors and destructor

	FORCEINLINE TRefCountPtr2():
		Reference(nullptr)
	{ }
	
	TRefCountPtr2(ReferencedType* InReference, bool bAddRef = true)
	{
		Reference = InReference;
		if(Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}
	
	TRefCountPtr2(const TRefCountPtr2& Copy)
	{
		Reference = Copy.Reference;
		if(Reference)
		{
			Reference->AddRef();
		}
	}

	template<typename CopyReferencedType>
	explicit TRefCountPtr2(const TRefCountPtr2<CopyReferencedType>& Copy)
	{
		Reference = static_cast<ReferencedType*>(Copy.Get());
		if (Reference)
		{
			Reference->AddRef();
		}
	}

	FORCEINLINE TRefCountPtr2(TRefCountPtr2&& Move) noexcept
	{
		Reference = Move.Reference;
		Move.Reference = nullptr;
	}

	template<typename MoveReferencedType>
	explicit TRefCountPtr2(TRefCountPtr2<MoveReferencedType>&& Move)
	{
		Reference = static_cast<ReferencedType*>(Move.Get());
		Move.Reference = nullptr;
	}

	void ReleaseReference(ReferencedType* inReference)
	{
		if (inReference)
		{
			bool needToFree = inReference->NeedAutoMemoryFree() && IsManaged;
			if (inReference->Release(IsManaged) == 0 && needToFree)
			{
				TMemory::Destroy(inReference);
			}
		}
	}

	~TRefCountPtr2()
	{
		ReleaseReference(Reference);
	}

	// access

	FORCEINLINE ReferencedType* operator->() const
	{
		return Reference;
	}

	FORCEINLINE operator ReferenceType() const
	{
		return Reference;
	}

	FORCEINLINE ReferencedType* Get() const
	{
		return Reference;
	}

	FORCEINLINE friend bool IsValidRef(const TRefCountPtr2& InReference)
	{
		return InReference.Reference != nullptr;
	}

	FORCEINLINE bool IsValid() const
	{
		return Reference != nullptr;
	}

	FORCEINLINE void SafeRelease()
	{
		*this = nullptr;
	}

	uint32 GetRefCount()
	{
		uint32 Result = 0;
		if (Reference)
		{
			Result = Reference->GetRefCount();
			TAssert(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer (*this is live)
		}
		return Result;
	}
	
	// operator=
	TRefCountPtr2& operator=(ReferencedType* InReference)
	{
		if (Reference != InReference)
		{
			// Call AddRef before Release, in case the new reference is the same as the old reference.
			ReferencedType* OldReference = Reference;
			Reference = InReference;
			if (Reference)
			{
				Reference->AddRef();
			}
			ReleaseReference(OldReference);
		}
		return *this;
	}

	FORCEINLINE TRefCountPtr2& operator=(const TRefCountPtr2& InPtr)
	{
		return *this = InPtr.Reference;
	}

	template<typename CopyReferencedType>
	FORCEINLINE TRefCountPtr2& operator=(const TRefCountPtr2<CopyReferencedType>& InPtr)
	{
		return *this = InPtr.Get();
	}

	TRefCountPtr2& operator=(TRefCountPtr2&& InPtr)
	{
		if (this != &InPtr)
		{
			ReferencedType* OldReference = Reference;
			Reference = InPtr.Reference;
			InPtr.Reference = nullptr;
			ReleaseReference(OldReference);
		}
		return *this;
	}

	template<typename MoveReferencedType>
	TRefCountPtr2& operator=(TRefCountPtr<MoveReferencedType>&& InPtr)
	{
		// InPtr is a different type (or we would have called the other operator), so we need not test &InPtr != this
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = nullptr;
		ReleaseReference(OldReference);
		return *this;
	}

	// comparison
	FORCEINLINE bool operator==(const TRefCountPtr2& B) const
	{
		return Get() == B.Get();
	}

	FORCEINLINE bool operator==(ReferencedType* B) const
	{
		return Get() == B;
	}
	
private:

	ReferencedType* Reference;

	template <typename OtherType, bool OtherIsManaged>
	friend class TRefCountPtr2;
};

template<typename ReferencedType>
FORCEINLINE bool operator==(ReferencedType* A, const TRefCountPtr<ReferencedType>& B)
{
	return A == B.Get();
}

template <typename T, bool isManaged = false, typename... TArgs>
FORCEINLINE TRefCountPtr2<T, isManaged> MakeRefCount2(TArgs&&... Args)
{
    if constexpr (isManaged)
    {
        return TRefCountPtr2<T, true>(new (TMemory::Malloc<T>()) T(std::forward<TArgs>(Args)...));
    }
    else
    {
        return TRefCountPtr2<T, false>(new T(std::forward<TArgs>(Args)...));
    }
}

class MyRefCountClass : public RefCountedObject
{
public:
    MyRefCountClass() = default;
    MyRefCountClass(int inX, char* inY, double inZ)
        : x(inX), y(inY), z(inZ)
    {
    }
    MyRefCountClass(int inX, double inZ)
        : x(inX), z(inZ)
    {
    }
protected:
    int x = 5;
    char* y = nullptr;
    double z = 0.0;
};

class MyRefCountClass1 : public MyRefCountClass
{
public:
    void Init()
    {
       // Member = MakeRefCount<MyRefCountClass>(1, 2.0);
    }
private:
   // TRefCountPtr<MyRefCountClass> Member;
};

std::vector<class MyUserDefinedRefCountClass*> GDeferredDeleteList{};
std::mutex GDeferredDeleteLock{};
class MyUserDefinedRefCountClass
{
public:
    MyUserDefinedRefCountClass() = default;
    MyUserDefinedRefCountClass(int inX, char* inY, double inZ)
        : x(inX), y(inY), z(inZ)
    {
    }
    MyUserDefinedRefCountClass(int inX, double inZ)
        : x(inX), z(inZ)
    {
    }
    
    uint32 AddRef() const
    {
        return NumRefs.fetch_add(1, std::memory_order_acquire);
    }
    uint32 Release(bool)
    {
        const uint32 CurrentRefCount = NumRefs.fetch_sub(1, std::memory_order_release);
        TAssert(CurrentRefCount > 0, "Reference is released more than once.");
        if (CurrentRefCount == 1)
        {
            std::lock_guard<std::mutex> synchronized{ GDeferredDeleteLock };
            GDeferredDeleteList.push_back(this);
            std::cout << "okay" << std::endl;
        }
        return CurrentRefCount - 1;
    }
    uint32 GetRefCount() const
    {
        return NumRefs.load(std::memory_order_acquire);
    }

    bool NeedAutoMemoryFree() const { return false; }
    
protected:
    int x = 5;
    char* y = nullptr;
    double z = 0.0;
	mutable std::atomic_uint NumRefs;
};

int main()
{
    ModuleManager::GetInstance()->LoadModule<CoreModule>();

    {
        //TRefCountPtr<MyRefCountClass1> a{ nullptr };
    	auto b = MakeRefCount<MyRefCountClass1>();
        // TRefCountPtr<MyRefCountClass> a = nullptr;
    	// a = b;
    	// a.operator=<MyRefCountClass1>(b);
        TRefCountPtr<MyRefCountClass> a = b;
        // TRefCountPtr<MyRefCountClass> c = b;
        //a->Init();
    }
	
    {
        size_t size = 0;
        int* ptr = new (TMemory::Malloc<int>()) int{ 5 };
        GMalloc->GetAllocationSize((void*)(ptr), size);
        std::cout << "Size = " << size << std::endl;
    }
	
    /*{
		// error
        size_t size = 0;
        void* ptr = reinterpret_cast<void*>(0x1234);
        try
        {
            GMalloc->GetAllocationSize((void*)(ptr), size);
        }
        catch (std::exception& except)
        {
        std::cout << "Size error" << std::endl;
        }
        std::cout << "Size = " << size << std::endl;
    }*/


    // Test ref-count.
    {
        // TODO 1 : This crashes.
        // Can not use GetAllocationSize to deduct weather the memory is allocated or not.
        // Add a template parameter to declare that this is a managed memory pointer.
        auto a = new MyRefCountClass;
        TRefCountPtr<MyRefCountClass> b = a;
    }
    {
        // TODO 2 : Check RefCountedObject::Release.
        auto a = new (TMemory::Malloc<MyRefCountClass>()) MyRefCountClass;
        TRefCountPtr<MyRefCountClass, true> b = a;
    }
    {
        // TODO 3 : Replace MakeRefCount with MakeRefCount2.
        // TODO 4 : Parameter "isManaged" for MakeRefCount2 should be false by default.
        TRefCountPtr<MyRefCountClass, true> a = MakeRefCount<MyRefCountClass, true>(1, 2.0);
        TRefCountPtr<MyRefCountClass, false> b = MakeRefCount<MyRefCountClass, false>(1, 2.0);
        TRefCountPtr<MyRefCountClass> c = MakeRefCount<MyRefCountClass>(1, 2.0);
    }
    {
        // TODO 5 : Review the following code snippet.
        // Test user defined ref-count object.
        {
            TRefCountPtr<MyUserDefinedRefCountClass,true> a = MakeRefCount<MyUserDefinedRefCountClass,true>(1, 2.0);
        }
        {
            // Deferred release.
            std::lock_guard<std::mutex> synchronized{ GDeferredDeleteLock };
            for (auto& itemToDelete : GDeferredDeleteList)
            {
			    TMemory::Destroy(itemToDelete);
            }
            GDeferredDeleteList.clear();
        }
    }

	ModuleManager::GetInstance()->UnloadModule<CoreModule>();
	ModuleManager::DestroyInstance();
    //system("pause");
    return 0;
}
#pragma optimize("", on)

