#include "Container/ReflectiveContainer.h"
#include "Vector.h"
#include <cstring>
#include <algorithm>

namespace Thunder
{
	template<typename T>
	ETypeKind GetTypeKind()
	{
		if constexpr (std::is_same_v<T, bool>) return ETypeKind::Bool;
		else if constexpr (std::is_same_v<T, int8>) return ETypeKind::Int8;
		else if constexpr (std::is_same_v<T, int16>) return ETypeKind::Int16;
		else if constexpr (std::is_same_v<T, int32>) return ETypeKind::Int32;
		else if constexpr (std::is_same_v<T, int64>) return ETypeKind::Int64;
		else if constexpr (std::is_same_v<T, uint8>) return ETypeKind::UInt8;
		else if constexpr (std::is_same_v<T, uint16>) return ETypeKind::UInt16;
		else if constexpr (std::is_same_v<T, uint32>) return ETypeKind::UInt32;
		else if constexpr (std::is_same_v<T, uint64>) return ETypeKind::UInt64;
		else if constexpr (std::is_same_v<T, float>) return ETypeKind::Float;
		else if constexpr (std::is_same_v<T, double>) return ETypeKind::Double;
		else if constexpr (std::is_same_v<T, TVector2f>) return ETypeKind::Vector2f;
		else if constexpr (std::is_same_v<T, TVector3f>) return ETypeKind::Vector3f;
		else if constexpr (std::is_same_v<T, TVector4f>) return ETypeKind::Vector4f;
		else if constexpr (std::is_same_v<T, TVector2d>) return ETypeKind::Vector2d;
		else if constexpr (std::is_same_v<T, TVector3d>) return ETypeKind::Vector3d;
		else if constexpr (std::is_same_v<T, TVector4d>) return ETypeKind::Vector4d;
		else if constexpr (std::is_same_v<T, TVector2i>) return ETypeKind::Vector2i;
		else if constexpr (std::is_same_v<T, TVector3i>) return ETypeKind::Vector3i;
		else if constexpr (std::is_same_v<T, TVector4i>) return ETypeKind::Vector4i;
		else if constexpr (std::is_same_v<T, TVector2u>) return ETypeKind::Vector2u;
		else if constexpr (std::is_same_v<T, TVector3u>) return ETypeKind::Vector3u;
		else if constexpr (std::is_same_v<T, TVector4u>) return ETypeKind::Vector4u;
		else return ETypeKind::Custom;
	}

	template<typename T>
	TypeComponent TypeComponent::Create(const String& inName, size_t inOffset)
	{
		TypeComponent component;
		component.Name = inName;
		component.Size = sizeof(T);
		component.Offset = inOffset;
		component.Kind = GetTypeKind<T>();

		// Set up function pointers for type operations
		component.Constructor = [](void* ptr) {
			new(ptr) T();
		};

		component.Destructor = [](void* ptr) {
			static_cast<T*>(ptr)->~T();
		};

		component.CopyConstructor = [](void* ptr, const void* src) {
			new(ptr) T(*static_cast<const T*>(src));
		};

		component.MoveConstructor = [](void* ptr, void* src) {
			new(ptr) T(std::move(*static_cast<T*>(src)));
		};

		component.CopyAssign = [](void* ptr, const void* src) {
			*static_cast<T*>(ptr) = *static_cast<const T*>(src);
		};

		component.MoveAssign = [](void* ptr, void* src) {
			*static_cast<T*>(ptr) = std::move(*static_cast<T*>(src));
		};

		return component;
	}

	ReflectiveContainer::ReflectiveContainer()
		: Data(nullptr)
		, Stride(0)
		, bInitialized(false)
	{
	}

	ReflectiveContainer::~ReflectiveContainer()
	{
		Clear();
	}

	ReflectiveContainer::ReflectiveContainer(const ReflectiveContainer& other)
		: Data(nullptr)
		, Components(other.Components)
		, Stride(other.Stride)
		, bInitialized(false)
	{
		if (other.bInitialized)
		{
			Initialize();
			CopyData(other);
		}
	}

	ReflectiveContainer::ReflectiveContainer(ReflectiveContainer&& other) noexcept
		: Data(other.Data)
		, Components(std::move(other.Components))
		, Stride(other.Stride)
		, bInitialized(other.bInitialized)
	{
		other.Data = nullptr;
		other.Stride = 0;
		other.bInitialized = false;
	}

	ReflectiveContainer& ReflectiveContainer::operator=(const ReflectiveContainer& other)
	{
		if (this != &other)
		{
			Clear();
			Components = other.Components;
			Stride = other.Stride;
			bInitialized = false;

			if (other.bInitialized)
			{
				Initialize();
				CopyData(other);
			}
		}
		return *this;
	}

	ReflectiveContainer& ReflectiveContainer::operator=(ReflectiveContainer&& other) noexcept
	{
		if (this != &other)
		{
			Clear();
			Data = other.Data;
			Components = std::move(other.Components);
			Stride = other.Stride;
			bInitialized = other.bInitialized;

			other.Data = nullptr;
			other.Stride = 0;
			other.bInitialized = false;
		}
		return *this;
	}

	template<typename T>
	void ReflectiveContainer::AddComponent(const String& name)
	{
		if (bInitialized)
		{
			return; // Cannot add components after initialization
		}

		TypeComponent component = TypeComponent::Create<T>(name, 0); // Offset will be calculated later
		Components.push_back(component);
	}

	void ReflectiveContainer::Initialize()
	{
		if (bInitialized)
		{
			return;
		}

		CalculateLayout();
		AllocateData();
		
		// Construct all components
		for (const auto& component : Components)
		{
			if (component.Constructor)
			{
				component.Constructor(static_cast<char*>(Data) + component.Offset);
			}
		}

		bInitialized = true;
	}

	void ReflectiveContainer::CopyData(const void* src, size_t offset, size_t size) const
	{
		memcpy(static_cast<uint8*>(Data) + offset, src, size);
	}

	template<typename T>
	T* ReflectiveContainer::GetComponent(const String& name)
	{
		size_t index = FindComponentIndex(name);
		if (index == SIZE_MAX || !bInitialized)
		{
			return nullptr;
		}

		const TypeComponent& component = Components[index];
		if (component.Kind != GetTypeKind<T>())
		{
			return nullptr; // Type mismatch
		}

		return reinterpret_cast<T*>(static_cast<char*>(Data) + component.Offset);
	}

	template<typename T>
	const T* ReflectiveContainer::GetComponent(const String& name) const
	{
		size_t index = FindComponentIndex(name);
		if (index == SIZE_MAX || !bInitialized)
		{
			return nullptr;
		}

		const TypeComponent& component = Components[index];
		if (component.Kind != GetTypeKind<T>())
		{
			return nullptr; // Type mismatch
		}

		return reinterpret_cast<const T*>(static_cast<const char*>(Data) + component.Offset);
	}

	template<typename T>
	void ReflectiveContainer::SetComponent(const String& name, const T& value)
	{
		T* component = GetComponent<T>(name);
		if (component)
		{
			*component = value;
		}
	}

	template<typename T>
	T* ReflectiveContainer::GetComponentByIndex(size_t index)
	{
		if (index >= Components.size() || !bInitialized)
		{
			return nullptr;
		}

		const TypeComponent& component = Components[index];
		if (component.Kind != GetTypeKind<T>())
		{
			return nullptr; // Type mismatch
		}

		return reinterpret_cast<T*>(static_cast<char*>(Data) + component.Offset);
	}

	template<typename T>
	const T* ReflectiveContainer::GetComponentByIndex(size_t index) const
	{
		if (index >= Components.size() || !bInitialized)
		{
			return nullptr;
		}

		const TypeComponent& component = Components[index];
		if (component.Kind != GetTypeKind<T>())
		{
			return nullptr; // Type mismatch
		}

		return reinterpret_cast<const T*>(static_cast<const char*>(Data) + component.Offset);
	}

	const TypeComponent* ReflectiveContainer::GetComponentInfo(const String& name) const
	{
		size_t index = FindComponentIndex(name);
		if (index == SIZE_MAX)
		{
			return nullptr;
		}
		return &Components[index];
	}

	const TypeComponent* ReflectiveContainer::GetComponentInfo(size_t index) const
	{
		if (index >= Components.size())
		{
			return nullptr;
		}
		return &Components[index];
	}

	void ReflectiveContainer::Clear()
	{
		if (bInitialized && Data)
		{
			DestroyData();
		}
		
		Components.clear();
		Stride = 0;
		bInitialized = false;
	}

	void ReflectiveContainer::CalculateLayout()
	{
		size_t currentOffset = 0;
		
		for (auto& component : Components)
		{
			component.Offset = currentOffset;
			currentOffset += component.Size;
		}
		
		Stride = currentOffset;
	}

	void ReflectiveContainer::AllocateData()
	{
		if (Stride > 0 && DataNum)
		{
			Data = std::malloc(Stride * DataNum);
		}
	}

	void ReflectiveContainer::DestroyData()
	{
		if (Data)
		{
			// Destroy components in reverse order
			for (auto it = Components.rbegin(); it != Components.rend(); ++it)
			{
				if (it->Destructor)
				{
					it->Destructor(static_cast<char*>(Data) + it->Offset);
				}
			}
			
			std::free(Data);
			Data = nullptr;
		}
	}

	void ReflectiveContainer::CopyData(const ReflectiveContainer& other)
	{
		if (!Data || !other.Data || Stride != other.Stride)
		{
			return;
		}

		// Copy each component individually using their copy constructors
		for (const auto& component : Components)
		{
			if (component.CopyConstructor)
			{
				component.CopyConstructor(
					static_cast<char*>(Data) + component.Offset,
					static_cast<const char*>(other.Data) + component.Offset
				);
			}
		}
	}

	size_t ReflectiveContainer::FindComponentIndex(const String& name) const
	{
		for (size_t i = 0; i < Components.size(); ++i)
		{
			if (Components[i].Name == name)
			{
				return i;
			}
		}
		return SIZE_MAX;
	}

	// Explicit template instantiations for common types
	template void ReflectiveContainer::AddComponent<bool>(const String& name);
	template void ReflectiveContainer::AddComponent<int8>(const String& name);
	template void ReflectiveContainer::AddComponent<int16>(const String& name);
	template void ReflectiveContainer::AddComponent<int32>(const String& name);
	template void ReflectiveContainer::AddComponent<int64>(const String& name);
	template void ReflectiveContainer::AddComponent<uint8>(const String& name);
	template void ReflectiveContainer::AddComponent<uint16>(const String& name);
	template void ReflectiveContainer::AddComponent<uint32>(const String& name);
	template void ReflectiveContainer::AddComponent<uint64>(const String& name);
	template void ReflectiveContainer::AddComponent<float>(const String& name);
	template void ReflectiveContainer::AddComponent<double>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector2f>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector3f>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector4f>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector2d>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector3d>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector4d>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector2i>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector3i>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector4i>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector2u>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector3u>(const String& name);
	template void ReflectiveContainer::AddComponent<TVector4u>(const String& name);

	// Template instantiations for GetComponent
	template bool* ReflectiveContainer::GetComponent<bool>(const String& name);
	template int8* ReflectiveContainer::GetComponent<int8>(const String& name);
	template int16* ReflectiveContainer::GetComponent<int16>(const String& name);
	template int32* ReflectiveContainer::GetComponent<int32>(const String& name);
	template int64* ReflectiveContainer::GetComponent<int64>(const String& name);
	template uint8* ReflectiveContainer::GetComponent<uint8>(const String& name);
	template uint16* ReflectiveContainer::GetComponent<uint16>(const String& name);
	template uint32* ReflectiveContainer::GetComponent<uint32>(const String& name);
	template uint64* ReflectiveContainer::GetComponent<uint64>(const String& name);
	template float* ReflectiveContainer::GetComponent<float>(const String& name);
	template double* ReflectiveContainer::GetComponent<double>(const String& name);
	template TVector2f* ReflectiveContainer::GetComponent<TVector2f>(const String& name);
	template TVector3f* ReflectiveContainer::GetComponent<TVector3f>(const String& name);
	template TVector4f* ReflectiveContainer::GetComponent<TVector4f>(const String& name);
	template TVector2d* ReflectiveContainer::GetComponent<TVector2d>(const String& name);
	template TVector3d* ReflectiveContainer::GetComponent<TVector3d>(const String& name);
	template TVector4d* ReflectiveContainer::GetComponent<TVector4d>(const String& name);
	template TVector2i* ReflectiveContainer::GetComponent<TVector2i>(const String& name);
	template TVector3i* ReflectiveContainer::GetComponent<TVector3i>(const String& name);
	template TVector4i* ReflectiveContainer::GetComponent<TVector4i>(const String& name);
	template TVector2u* ReflectiveContainer::GetComponent<TVector2u>(const String& name);
	template TVector3u* ReflectiveContainer::GetComponent<TVector3u>(const String& name);
	template TVector4u* ReflectiveContainer::GetComponent<TVector4u>(const String& name);
}