#pragma once

#include "Platform.h"
#include "Templates/RefCountObject.h"
#include "Memory/MemoryBase.h"

namespace Thunder
{
	template<typename ReferencedType, bool IsManaged = false>
	class TRefCountPtr
	{
		typedef ReferencedType* ReferenceType;

	public:
		// constructors and destructor

		FORCEINLINE TRefCountPtr():
			Reference(nullptr)
		{ }
		
		TRefCountPtr(ReferencedType* InReference, bool bAddRef = true)
		{
			Reference = InReference;
			if(Reference && bAddRef)
			{
				Reference->AddRef();
			}
		}
		
		TRefCountPtr(const TRefCountPtr& Copy)
		{
			Reference = Copy.Reference;
			if(Reference)
			{
				Reference->AddRef();
			}
		}

		template<typename CopyReferencedType>
		explicit TRefCountPtr(const TRefCountPtr<CopyReferencedType, IsManaged>& Copy)
		{
			Reference = static_cast<ReferencedType*>(Copy.Get());
			if (Reference)
			{
				Reference->AddRef();
			}
		}

		FORCEINLINE TRefCountPtr(TRefCountPtr&& Move) noexcept
		{
			Reference = Move.Reference;
			Move.Reference = nullptr;
		}

		template<typename MoveReferencedType>
		explicit TRefCountPtr(TRefCountPtr<MoveReferencedType, IsManaged>&& Move)
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

		~TRefCountPtr()
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

		FORCEINLINE friend bool IsValidRef(const TRefCountPtr& InReference)
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
		TRefCountPtr& operator=(ReferencedType* InReference)
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

		FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr& InPtr)
		{
			return *this = InPtr.Reference;
		}

		template<typename CopyReferencedType>
		FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr<CopyReferencedType, IsManaged>& InPtr)
		{
			return *this = InPtr.Get();
		}

		TRefCountPtr& operator=(TRefCountPtr&& InPtr)
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
		TRefCountPtr& operator=(TRefCountPtr<MoveReferencedType, IsManaged>&& InPtr)
		{
			// InPtr is a different type (or we would have called the other operator), so we need not test &InPtr != this
			ReferencedType* OldReference = Reference;
			Reference = InPtr.Reference;
			InPtr.Reference = nullptr;
			ReleaseReference(OldReference);
			return *this;
		}

		// comparison
		FORCEINLINE bool operator==(const TRefCountPtr& B) const
		{
			return Get() == B.Get();
		}

		FORCEINLINE bool operator==(ReferencedType* B) const
		{
			return Get() == B;
		}

		// conversion
		template <typename CastedType>
		operator TRefCountPtr<CastedType, IsManaged>() const
		{
			return TRefCountPtr<CastedType, IsManaged>(Reference);
		}
		
	private:

		ReferencedType* Reference;

		template <typename OtherType, bool OtherIsManaged>
		friend class TRefCountPtr;
	};

	template<typename ReferencedType>
	FORCEINLINE bool operator==(ReferencedType* A, const TRefCountPtr<ReferencedType>& B)
	{
		return A == B.Get();
	}

	template <typename T, bool isManaged = false, typename... TArgs>
	FORCEINLINE CORE_API TRefCountPtr<T, isManaged> MakeRefCount(TArgs&&... Args)
	{
		if constexpr (isManaged)
		{
			return TRefCountPtr<T, true>(new (TMemory::Malloc<T>()) T(std::forward<TArgs>(Args)...));
		}
		else
		{
			return TRefCountPtr<T, false>(new T(std::forward<TArgs>(Args)...));
		}
	}
}

