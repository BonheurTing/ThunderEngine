#pragma once
#pragma optimize("", off)
#include <cstring>
#include <type_traits>
#include "Platform.h"
#include "Assertion.h"
#include "Memory/MemoryBase.h"
#define NUM_TFUNCTION_INLINE_BYTES 32
#define TFUNCTION_INLINE_SIZE         NUM_TFUNCTION_INLINE_BYTES
#define TFUNCTION_INLINE_ALIGNMENT    16

template <typename T> struct TRemovePointer     { typedef T Type; };
template <typename T> struct TRemovePointer<T*> { typedef T Type; };


namespace Thunder
{
	template<int32 Size, uint32 Alignment>
	struct TAlignedBytes
	{
		alignas(Alignment) uint8 Pad[Size];
	};

	template <typename T>
	FORCEINLINE std::remove_reference_t<T>&& MoveTemp(T&& Obj)
	{
		using CastType = std::remove_reference_t<T>;

		// Validate that we're not being passed an rvalue or a const object - the former is redundant, the latter is almost certainly a mistake
		static_assert(std::is_lvalue_reference_v<T>, "MoveTemp called on an rvalue");
		static_assert(!std::is_same_v<CastType&, const CastType&>, "MoveTemp called on a const object");

		return (CastType&&)Obj;
	}

	struct IFunction_OwnedObject
	{
		virtual void* CloneToEmptyStorage(void* Storage) const = 0;
		virtual void* GetAddress() = 0;
		virtual void Destroy() = 0;
		virtual ~IFunction_OwnedObject() = default;
	};

	template <typename T>
	struct IFunction_OwnedObject_OnHeap : public IFunction_OwnedObject
	{
		virtual void Destroy() override
		{
			void* This = this;
			this->~IFunction_OwnedObject_OnHeap();
			TMemory::Free(This);
		}

		~IFunction_OwnedObject_OnHeap() override {}
	};

	template <typename T>
	struct IFunction_OwnedObject_Inline : public IFunction_OwnedObject
	{
		virtual void Destroy() override
		{
			this->~IFunction_OwnedObject_Inline();
		}

		~IFunction_OwnedObject_Inline() override {}
	};

	template <typename T, bool bOnHeap>
	struct TFunction_OwnedObject : public
		std::conditional_t<bOnHeap, IFunction_OwnedObject_OnHeap<T>, IFunction_OwnedObject_Inline<T>>
	{
		~TFunction_OwnedObject()
		{
		}
		
		template <typename... ArgTypes>
		explicit TFunction_OwnedObject(ArgTypes&&... Args)
			: Obj(std::forward<ArgTypes>(Args)...)
		{
		}

		void* CloneToEmptyStorage(void* Storage) const override
		{
			TAssert(false);
			return nullptr;
		}

		void Destroy() override
		{
			this->~TFunction_OwnedObject();
		}

		void* GetAddress() override
		{
			return &Obj;
		}

		T Obj;
	};

	template <typename FunctorType, bool bOnHeap>
	struct TStorageOwnerType
	{
		using Type = TFunction_OwnedObject<std::decay_t<FunctorType>, bOnHeap>;
	};

	template <typename FunctorType, bool bOnHeap>
	using TStorageOwnerTypeT = typename TStorageOwnerType<FunctorType, bOnHeap>::Type;

	//TFunctionStorage + FFunctionStorage
	struct FunctionStorage
	{
		FunctionStorage()
			: HeapAllocation(nullptr)
		{
		}

		FunctionStorage(FunctionStorage&& Other)
			: HeapAllocation(Other.HeapAllocation)
		{
			Other.HeapAllocation = nullptr;
			memcpy(&InlineAllocation, &Other.InlineAllocation, sizeof(InlineAllocation));
		}

		FunctionStorage(const FunctionStorage& Other) = delete;
		FunctionStorage& operator=(FunctionStorage&& Other) = delete;
		FunctionStorage& operator=(const FunctionStorage& Other) = delete;

		void* BindCopy(const FunctionStorage& Other)
		{
			void* NewObj = Other.GetBoundObject()->CloneToEmptyStorage(this);
			return NewObj;
		}

		_NODISCARD_ IFunction_OwnedObject* GetBoundObject() const
		{
			const auto Result = static_cast<IFunction_OwnedObject*>(HeapAllocation);
			return Result;
		}
		
		//Returns a pointer to the callable object
		void* GetPtr() const
		{
			auto Owned = static_cast<IFunction_OwnedObject*>(HeapAllocation);
			if (!Owned)
			{
				Owned = (IFunction_OwnedObject*)(&InlineAllocation);
			}

			return Owned->GetAddress();

		}

		template <typename FunctorType>
		std::decay_t<FunctorType>* Bind(FunctorType&& InFunc)
		{
			/*if (!IsBound(InFunc))
			{
				return nullptr;
			}*/

			// 算字节
			constexpr bool bUseInline = sizeof(TStorageOwnerTypeT<FunctorType, false>) <= TFUNCTION_INLINE_SIZE;
			using OwnedType = TStorageOwnerTypeT<FunctorType, !bUseInline>;

			void* NewAlloc;
			if constexpr (bUseInline)
			{
				NewAlloc = &InlineAllocation;
			}
			else
			{
				NewAlloc = TMemory::Malloc(sizeof(OwnedType), alignof(OwnedType));
				HeapAllocation = NewAlloc;
			}

			auto* NewOwned = new (NewAlloc) OwnedType(std::forward<FunctorType>(InFunc));
			return &NewOwned->Obj;
		}

		void* HeapAllocation;
		TAlignedBytes<TFUNCTION_INLINE_SIZE, TFUNCTION_INLINE_ALIGNMENT> InlineAllocation;
	};

	/**
	 * A class which is used to instantiate the code needed to call a bound function.
	 */
	template <typename Functor, typename FuncType>
	struct TFunctionRefCaller;

	template <typename Functor, typename Ret, typename... ParamTypes>
	struct TFunctionRefCaller<Functor, Ret (ParamTypes...)>
	{
		static Ret Call(void* Obj, ParamTypes&... Params)
		{
			return std::forward<Functor>(*(Functor*)Obj)(std::forward<ParamTypes>(Params)...);
		}
	};

	template <typename Functor, typename... ParamTypes>
	struct TFunctionRefCaller<Functor, void (ParamTypes...)>
	{
		static void Call(void* Obj, ParamTypes&... Params)
		{
			std::forward<Functor>(*(Functor*)Obj)(std::forward<ParamTypes>(Params)...);
		}
	};

	template <typename Functor>
	struct TFunctionRefCaller<Functor, void (void)>
	{
		static void Call(void* Obj)
		{
			*(Functor*)(Obj);
		}
	};

	//TFunctionRefBase
	template <typename FuncType>
	class TFunctionMy;
	
	/**
	 * 写一遍std::function
	 * 功能一：构造时可存储函数指针和参数，并根据参数大小选择直接拷到栈上还是全局内存分配器拷到堆上
	 * 功能二：正常调用
	**/
	template <typename Ret, typename... ParamTypes>
	class TFunctionMy<Ret (ParamTypes...)>
	{
	public:
		template <typename OtherFuncType>
		friend class TFunctionMy;

		TFunctionMy() noexcept {}
		TFunctionMy(nullptr_t) noexcept {}
		

		template <typename FunctorType>
		TFunctionMy(const FunctorType& Other)
			: Callable(Other.Callable)
		{
			if (!Callable)
			{
				return;
			}
			void* NewPtr = Storage.BindCopy(Other.Storage);
		}
		
		template <typename FunctorType>
		TFunctionMy(FunctorType&& InFunc)
		{
			if (!Storage.Bind(std::forward<FunctorType>(InFunc)))
			{
				TAssertf(false, "Invalid function binding");
				return; 
			}
			
			Callable = &TFunctionRefCaller<std::decay_t<FunctorType>, Ret (ParamTypes...)>::Call;
		}

		TFunctionMy& operator=(TFunctionMy&&) = delete;
		TFunctionMy& operator=(const TFunctionMy&) = delete;
		
		Ret operator()(ParamTypes... Params) const
		{
			return Callable(Storage.GetPtr(), Params...);
		}

	private:
		Ret (*Callable)(void*, ParamTypes&...) = nullptr;
		FunctionStorage Storage;
	};


	//AsyncTask(
	template <typename Ret, typename... ParamTypes>
	static Ret AsyncTask(TFunctionMy<Ret (ParamTypes...)>& FunctionType, ParamTypes... Params)
	{
		return FunctionType(Params...);
	}

	
}
