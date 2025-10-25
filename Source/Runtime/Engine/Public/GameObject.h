#pragma once
#include "BasicDefinition.h"
#include "Guid.h"
#include "NameHandle.h"
#include "FileSystem/MemoryArchive.h"
#include <atomic>

namespace Thunder
{
	// Forward declarations
	class GameObject;
	template<typename T> class TStrongObjectPtr;
	template<typename T> class TWeakObjectPtr;

	// Control block for GameObject reference counting
	struct GameObjectControlBlock
	{
		std::atomic<int> StrongRefCount { 0 }; // Count > 0 prevents GC
		std::atomic<int> WeakRefCount { 0 };   // Keeps control block alive
		std::atomic<bool> bAlive { true };     // Indicates if object is GC'd
		GameObject* ObjectPtr { nullptr };     // Pointer to the GameObject

		GameObjectControlBlock() = default;

		void AddStrongRef()
		{
			StrongRefCount.fetch_add(1, std::memory_order_relaxed);
		}

		void ReleaseStrongRef()
		{
			if (StrongRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				// Strong ref count reached zero, object can be GC'd
				// Note: We don't delete the object here, GC system will handle it
			}
		}

		void AddWeakRef()
		{
			WeakRefCount.fetch_add(1, std::memory_order_relaxed);
		}

		void ReleaseWeakRef()
		{
			if (WeakRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				// No more weak references, safe to delete control block
				if (!bAlive.load(std::memory_order_acquire))
				{
					delete this;
				}
			}
		}

		void MarkAsDestroyed()
		{
			bAlive.store(false, std::memory_order_release);
			ObjectPtr = nullptr;

			// If no weak refs exist, delete control block
			if (WeakRefCount.load(std::memory_order_acquire) == 0)
			{
				delete this;
			}
		}
	};

	class GameObject
	{
	public:
		GameObject(GameObject* inOuter = nullptr) : Outer(inOuter) {}
		virtual ~GameObject()
		{
			if (ControlBlock)
			{
				ControlBlock->MarkAsDestroyed();
			}
		}
		_NODISCARD_ GameObject* GetOuter() const { return Outer; }
		_NODISCARD_ GameObjectControlBlock* GetControlBlock() const { return ControlBlock; }

	private:
		friend GameObjectControlBlock* AllocateGameObjectWithControlBlock(size_t objectSize, size_t objectAlignment);
		template<typename T, typename... Args> friend TStrongObjectPtr<T> MakeGameObject(Args&&... args);

		void SetControlBlock(GameObjectControlBlock* block) { ControlBlock = block; }

	private:
		GameObject* Outer { nullptr };
		GameObjectControlBlock* ControlBlock { nullptr };
	};

	enum class ETempGameResourceReflective : uint32
	{
		StaticMesh = 0,
		Texture2D,
		Material,
		Unknown
	};

	class GameResource : public GameObject
	{
	public:
		GameResource(GameObject* inOuter = nullptr, ETempGameResourceReflective inType = ETempGameResourceReflective::Unknown)
			: GameObject(inOuter), ResourceType(inType)
		{
			TGuid::GenerateGUID(Guid);
		}
		_NODISCARD_ TGuid GetGUID() const { return Guid; }
		_NODISCARD_ ETempGameResourceReflective GetResourceType() const { return ResourceType; }
		_NODISCARD_ const NameHandle& GetResourceName() const { return ResourceName; }
		void AddDependency(const TGuid& guid) { Dependencies.push_back(guid); }
		_NODISCARD_ void GetDependencies(TArray<TGuid>& outDependencies) const { outDependencies = Dependencies; }
		void SetResourceName(const NameHandle& name) { ResourceName = name; }

		virtual void Serialize(MemoryWriter& archive);
		virtual void DeSerialize(MemoryReader& archive);
		virtual void OnResourceLoaded()
		{
			LOG("success OnResourceLoaded");
		}

	private:
		friend class Package;
		void SetGuid(const TGuid& guid) { Guid = guid; }

	private:
		TGuid Guid {};
		ETempGameResourceReflective ResourceType { ETempGameResourceReflective::Unknown };
		NameHandle ResourceName {}; // Virtual path of resources (like "/Game/Textures.Texture1")
		TArray<TGuid> Dependencies {}; // The GUID of other resources required by this resource needs to be loaded together when it is being loaded.
	};

	enum class LoadingStatus : uint8
	{
		Idle,
		Loading,
		Loaded
	};

	class ITickable
	{
	public:
		virtual ~ITickable() = default;
		virtual void Tick() = 0;
	};

	// ===========================================
	// TStrongObjectPtr - Strong reference smart pointer
	// ===========================================
	template<typename T>
	class TStrongObjectPtr
	{
		static_assert(std::is_base_of_v<GameObject, T>, "TStrongObjectPtr can only be used with GameObject-derived types");

	public:
		TStrongObjectPtr() : ControlBlock(nullptr) {}

		TStrongObjectPtr(std::nullptr_t) : ControlBlock(nullptr) {}

		// Constructor from raw pointer (assumes object has a control block)
		TStrongObjectPtr(T* inObject)
			: ControlBlock(inObject ? inObject->GetControlBlock() : nullptr)
		{
			if (ControlBlock)
			{
				ControlBlock->AddStrongRef();
				ControlBlock->AddWeakRef(); // Strong ptr also holds weak ref to keep control block alive
			}
		}

		// Copy constructor
		TStrongObjectPtr(const TStrongObjectPtr& other)
			: ControlBlock(other.ControlBlock)
		{
			if (ControlBlock)
			{
				ControlBlock->AddStrongRef();
				ControlBlock->AddWeakRef();
			}
		}

		// Move constructor
		TStrongObjectPtr(TStrongObjectPtr&& other) noexcept
			: ControlBlock(other.ControlBlock)
		{
			other.ControlBlock = nullptr;
		}

		// Copy constructor from compatible type
		template<typename U, typename = typename TEnableIf<std::is_base_of_v<T, U> || std::is_base_of_v<U, T>>::Type>
		TStrongObjectPtr(const TStrongObjectPtr<U>& other)
			: ControlBlock(other.ControlBlock)
		{
			if (ControlBlock)
			{
				ControlBlock->AddStrongRef();
				ControlBlock->AddWeakRef();
			}
		}

		~TStrongObjectPtr()
		{
			Reset();
		}

		// Copy assignment
		TStrongObjectPtr& operator=(const TStrongObjectPtr& other)
		{
			if (this != &other)
			{
				Reset();
				ControlBlock = other.ControlBlock;
				if (ControlBlock)
				{
					ControlBlock->AddStrongRef();
					ControlBlock->AddWeakRef();
				}
			}
			return *this;
		}

		// Move assignment
		TStrongObjectPtr& operator=(TStrongObjectPtr&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				ControlBlock = other.ControlBlock;
				other.ControlBlock = nullptr;
			}
			return *this;
		}

		// Reset to nullptr
		void Reset()
		{
			if (ControlBlock)
			{
				ControlBlock->ReleaseStrongRef();
				ControlBlock->ReleaseWeakRef();
				ControlBlock = nullptr;
			}
		}

		// Get raw pointer
		_NODISCARD_ T* Get() const
		{
			if (ControlBlock && ControlBlock->bAlive.load(std::memory_order_acquire))
			{
				return static_cast<T*>(ControlBlock->ObjectPtr);
			}
			return nullptr;
		}

		// Dereference operators
		_NODISCARD_ T* operator->() const { return Get(); }
		_NODISCARD_ T& operator*() const { return *Get(); }

		// Bool conversion
		explicit operator bool() const { return Get() != nullptr; }

		// Comparison operators
		bool operator==(const TStrongObjectPtr& other) const { return Get() == other.Get(); }
		bool operator!=(const TStrongObjectPtr& other) const { return Get() != other.Get(); }
		bool operator==(std::nullptr_t) const { return Get() == nullptr; }
		bool operator!=(std::nullptr_t) const { return Get() != nullptr; }

		// Get strong reference count
		_NODISCARD_ int GetStrongRefCount() const
		{
			return ControlBlock ? ControlBlock->StrongRefCount.load(std::memory_order_relaxed) : 0;
		}

	private:
		template<typename U> friend class TStrongObjectPtr;
		template<typename U> friend class TWeakObjectPtr;
		template<typename U, typename... Args> friend TStrongObjectPtr<U> MakeGameObject(Args&&... args);

		GameObjectControlBlock* ControlBlock;
	};

	// ===========================================
	// TWeakObjectPtr - Weak reference smart pointer
	// ===========================================
	template<typename T>
	class TWeakObjectPtr
	{
		static_assert(std::is_base_of_v<GameObject, T>, "TWeakObjectPtr can only be used with GameObject-derived types");

	public:
		TWeakObjectPtr() : ControlBlock(nullptr) {}

		TWeakObjectPtr(std::nullptr_t) : ControlBlock(nullptr) {}

		TWeakObjectPtr(T* inObject)
			: ControlBlock(inObject ? inObject->GetControlBlock() : nullptr)
		{
			if (ControlBlock)
			{
				ControlBlock->AddWeakRef();
			}
		}

		TWeakObjectPtr(const TStrongObjectPtr<T>& strongPtr)
			: ControlBlock(strongPtr.ControlBlock)
		{
			if (ControlBlock)
			{
				ControlBlock->AddWeakRef();
			}
		}

		TWeakObjectPtr(const TWeakObjectPtr& other)
			: ControlBlock(other.ControlBlock)
		{
			if (ControlBlock)
			{
				ControlBlock->AddWeakRef();
			}
		}

		TWeakObjectPtr(TWeakObjectPtr&& other) noexcept
			: ControlBlock(other.ControlBlock)
		{
			other.ControlBlock = nullptr;
		}

		template<typename U, typename = typename TEnableIf<std::is_base_of_v<T, U> || std::is_base_of_v<U, T>>::Type>
		TWeakObjectPtr(const TWeakObjectPtr<U>& other)
			: ControlBlock(other.ControlBlock)
		{
			if (ControlBlock)
			{
				ControlBlock->AddWeakRef();
			}
		}

		~TWeakObjectPtr()
		{
			Reset();
		}

		TWeakObjectPtr& operator=(const TWeakObjectPtr& other)
		{
			if (this != &other)
			{
				Reset();
				ControlBlock = other.ControlBlock;
				if (ControlBlock)
				{
					ControlBlock->AddWeakRef();
				}
			}
			return *this;
		}

		TWeakObjectPtr& operator=(TWeakObjectPtr&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				ControlBlock = other.ControlBlock;
				other.ControlBlock = nullptr;
			}
			return *this;
		}

		TWeakObjectPtr& operator=(const TStrongObjectPtr<T>& strongPtr)
		{
			Reset();
			ControlBlock = strongPtr.ControlBlock;
			if (ControlBlock)
			{
				ControlBlock->AddWeakRef();
			}
			return *this;
		}

		void Reset()
		{
			if (ControlBlock)
			{
				ControlBlock->ReleaseWeakRef();
				ControlBlock = nullptr;
			}
		}

		// Check if object is still alive
		_NODISCARD_ bool IsValid() const
		{
			return ControlBlock && ControlBlock->bAlive.load(std::memory_order_acquire);
		}

		// Get raw pointer (returns nullptr if object was destroyed)
		_NODISCARD_ T* Get() const
		{
			if (IsValid())
			{
				return static_cast<T*>(ControlBlock->ObjectPtr);
			}
			return nullptr;
		}

		// Lock to get a strong pointer
		_NODISCARD_ TStrongObjectPtr<T> Lock() const
		{
			if (IsValid())
			{
				return TStrongObjectPtr<T>(Get());
			}
			return TStrongObjectPtr<T>();
		}

		// Dereference operators (use with caution, check IsValid() first)
		_NODISCARD_ T* operator->() const { return Get(); }
		_NODISCARD_ T& operator*() const { return *Get(); }

		// Bool conversion
		explicit operator bool() const { return IsValid(); }

		// Comparison operators
		bool operator==(const TWeakObjectPtr& other) const { return ControlBlock == other.ControlBlock; }
		bool operator!=(const TWeakObjectPtr& other) const { return ControlBlock != other.ControlBlock; }
		bool operator==(std::nullptr_t) const { return !IsValid(); }
		bool operator!=(std::nullptr_t) const { return IsValid(); }

	private:
		template<typename U> friend class TWeakObjectPtr;
		template<typename U> friend class TStrongObjectPtr;

		GameObjectControlBlock* ControlBlock;
	};

	// ===========================================
	// MakeGameObject - Factory function
	// ===========================================
	// Allocates GameObject and control block contiguously in memory (like make_shared)
	template<typename T, typename... Args>
	TStrongObjectPtr<T> MakeGameObject(Args&&... args)
	{
		static_assert(std::is_base_of_v<GameObject, T>, "MakeGameObject can only be used with GameObject-derived types");

		// Calculate sizes and alignment
		constexpr size_t controlBlockSize = sizeof(GameObjectControlBlock);
		constexpr size_t objectSize = sizeof(T);
		constexpr size_t controlBlockAlign = alignof(GameObjectControlBlock);
		constexpr size_t objectAlign = alignof(T);

		// Use maximum alignment
		constexpr size_t maxAlign = controlBlockAlign > objectAlign ? controlBlockAlign : objectAlign;

		// Align control block size to object alignment
		constexpr size_t alignedControlBlockSize = (controlBlockSize + objectAlign - 1) & ~(objectAlign - 1);

		// Allocate memory for both control block and object
		void* memory = ::operator new(alignedControlBlockSize + objectSize, std::align_val_t(maxAlign));

		// Construct control block at the beginning
		GameObjectControlBlock* controlBlock = new(memory) GameObjectControlBlock();

		// Construct object after control block
		void* objectMemory = static_cast<char*>(memory) + alignedControlBlockSize;
		T* object = new(objectMemory) T(std::forward<Args>(args)...);

		// Link control block and object
		controlBlock->ObjectPtr = object;
		object->SetControlBlock(controlBlock);

		// Create strong pointer (this will add strong and weak ref)
		return TStrongObjectPtr<T>(object);
	}
}
