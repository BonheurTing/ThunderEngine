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

		SetupFunctionsForType<T>(component);

		return component;
	}

	template <typename T>
	void TypeComponent::SetupFunctionsForType(TypeComponent& component)
	{
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

	void ReflectiveContainer::Serialize(MemoryWriter& archive)
	{
		archive << static_cast<uint32>(Components.size());
		
		for (const auto& component : Components)
		{
			archive << component.Name;
			archive << component.Size;
			archive << component.Offset;
			archive << static_cast<uint8>(component.Kind);
		}
		
		archive << Stride;
		archive << DataNum;
		archive << bInitialized;
		
		if (bInitialized && Data && DataNum > 0)
		{
			archive << static_cast<uint8>(1);
			size_t totalDataSize = Stride * DataNum;
			archive << totalDataSize;
			
			for (size_t i = 0; i < DataNum; ++i)
			{
				const char* elementData = static_cast<const char*>(Data) + (i * Stride);
				
				for (const auto& component : Components)
				{
					const void* componentData = elementData + component.Offset;
					SerializeComponent(archive, componentData, component);
				}
			}
		}
		else
		{
			archive << static_cast<uint8>(0);
		}
	}

	void ReflectiveContainer::DeSerialize(MemoryReader& archive)
	{
		Clear();
		
		uint32 componentCount;
		archive >> componentCount;
		
		Components.resize(componentCount);
		for (uint32 i = 0; i < componentCount; ++i)
		{
			archive >> Components[i].Name;
			archive >> Components[i].Size;
			archive >> Components[i].Offset;
			
			uint8 kindValue;
			archive >> kindValue;
			Components[i].Kind = static_cast<ETypeKind>(kindValue);

			
			SetupComponentFunctions(Components[i]);
		}
		
		archive >> Stride;
		archive >> DataNum;
		archive >> bInitialized;
		
		uint8 hasData;
		archive >> hasData;
		
		if (hasData && DataNum > 0)
		{
			size_t totalDataSize;
			archive >> totalDataSize;
			
			AllocateData();
			
			for (size_t i = 0; i < DataNum; ++i)
			{
				char* elementData = static_cast<char*>(Data) + (i * Stride);
				
				for (const auto& component : Components)
				{
					void* componentData = elementData + component.Offset;
					
					if (component.Constructor)
					{
						component.Constructor(componentData);
					}
					
					DeserializeComponent(archive, componentData, component);
				}
			}
		}
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

	void ReflectiveContainer::SerializeComponent(MemoryWriter& archive, const void* componentData, const TypeComponent& component) const
	{
		switch (component.Kind)
		{
		case ETypeKind::Bool:
			archive << *static_cast<const bool*>(componentData);
			break;
		case ETypeKind::Int8:
			archive << *static_cast<const int8*>(componentData);
			break;
		case ETypeKind::Int16:
			archive << *static_cast<const int16*>(componentData);
			break;
		case ETypeKind::Int32:
			archive << *static_cast<const int32*>(componentData);
			break;
		case ETypeKind::Int64:
			archive << *static_cast<const int64*>(componentData);
			break;
		case ETypeKind::UInt8:
			archive << *static_cast<const uint8*>(componentData);
			break;
		case ETypeKind::UInt16:
			archive << *static_cast<const uint16*>(componentData);
			break;
		case ETypeKind::UInt32:
			archive << *static_cast<const uint32*>(componentData);
			break;
		case ETypeKind::UInt64:
			archive << *static_cast<const uint64*>(componentData);
			break;
		case ETypeKind::Float:
			archive << *static_cast<const float*>(componentData);
			break;
		case ETypeKind::Double:
			archive << *static_cast<const double*>(componentData);
			break;
		case ETypeKind::Vector2f:
			{
				const TVector2f& vec = *static_cast<const TVector2f*>(componentData);
				archive << vec.X << vec.Y;
			}
			break;
		case ETypeKind::Vector3f:
			{
				const TVector3f& vec = *static_cast<const TVector3f*>(componentData);
				archive << vec.X << vec.Y << vec.Z;
			}
			break;
		case ETypeKind::Vector4f:
			{
				const TVector4f& vec = *static_cast<const TVector4f*>(componentData);
				archive << vec.X << vec.Y << vec.Z << vec.W;
			}
			break;
		case ETypeKind::Vector2d:
			{
				const TVector2d& vec = *static_cast<const TVector2d*>(componentData);
				archive << vec.X << vec.Y;
			}
			break;
		case ETypeKind::Vector3d:
			{
				const TVector3d& vec = *static_cast<const TVector3d*>(componentData);
				archive << vec.X << vec.Y << vec.Z;
			}
			break;
		case ETypeKind::Vector4d:
			{
				const TVector4d& vec = *static_cast<const TVector4d*>(componentData);
				archive << vec.X << vec.Y << vec.Z << vec.W;
			}
			break;
		case ETypeKind::Vector2i:
			{
				const TVector2i& vec = *static_cast<const TVector2i*>(componentData);
				archive << vec.X << vec.Y;
			}
			break;
		case ETypeKind::Vector3i:
			{
				const TVector3i& vec = *static_cast<const TVector3i*>(componentData);
				archive << vec.X << vec.Y << vec.Z;
			}
			break;
		case ETypeKind::Vector4i:
			{
				const TVector4i& vec = *static_cast<const TVector4i*>(componentData);
				archive << vec.X << vec.Y << vec.Z << vec.W;
			}
			break;
		case ETypeKind::Vector2u:
			{
				const TVector2u& vec = *static_cast<const TVector2u*>(componentData);
				archive << vec.X << vec.Y;
			}
			break;
		case ETypeKind::Vector3u:
			{
				const TVector3u& vec = *static_cast<const TVector3u*>(componentData);
				archive << vec.X << vec.Y << vec.Z;
			}
			break;
		case ETypeKind::Vector4u:
			{
				const TVector4u& vec = *static_cast<const TVector4u*>(componentData);
				archive << vec.X << vec.Y << vec.Z << vec.W;
			}
			break;
		case ETypeKind::Custom:
		default:
			break;
		}
	}

	void ReflectiveContainer::DeserializeComponent(MemoryReader& archive, void* componentData, const TypeComponent& component) const
	{
		switch (component.Kind)
		{
		case ETypeKind::Bool:
			archive >> *static_cast<bool*>(componentData);
			break;
		case ETypeKind::Int8:
			archive >> *static_cast<int8*>(componentData);
			break;
		case ETypeKind::Int16:
			archive >> *static_cast<int16*>(componentData);
			break;
		case ETypeKind::Int32:
			archive >> *static_cast<int32*>(componentData);
			break;
		case ETypeKind::Int64:
			archive >> *static_cast<int64*>(componentData);
			break;
		case ETypeKind::UInt8:
			archive >> *static_cast<uint8*>(componentData);
			break;
		case ETypeKind::UInt16:
			archive >> *static_cast<uint16*>(componentData);
			break;
		case ETypeKind::UInt32:
			archive >> *static_cast<uint32*>(componentData);
			break;
		case ETypeKind::UInt64:
			archive >> *static_cast<uint64*>(componentData);
			break;
		case ETypeKind::Float:
			archive >> *static_cast<float*>(componentData);
			break;
		case ETypeKind::Double:
			archive >> *static_cast<double*>(componentData);
			break;
		case ETypeKind::Vector2f:
			{
				TVector2f& vec = *static_cast<TVector2f*>(componentData);
				archive >> vec.X >> vec.Y;
			}
			break;
		case ETypeKind::Vector3f:
			{
				TVector3f& vec = *static_cast<TVector3f*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z;
			}
			break;
		case ETypeKind::Vector4f:
			{
				TVector4f& vec = *static_cast<TVector4f*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z >> vec.W;
			}
			break;
		case ETypeKind::Vector2d:
			{
				TVector2d& vec = *static_cast<TVector2d*>(componentData);
				archive >> vec.X >> vec.Y;
			}
			break;
		case ETypeKind::Vector3d:
			{
				TVector3d& vec = *static_cast<TVector3d*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z;
			}
			break;
		case ETypeKind::Vector4d:
			{
				TVector4d& vec = *static_cast<TVector4d*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z >> vec.W;
			}
			break;
		case ETypeKind::Vector2i:
			{
				TVector2i& vec = *static_cast<TVector2i*>(componentData);
				archive >> vec.X >> vec.Y;
			}
			break;
		case ETypeKind::Vector3i:
			{
				TVector3i& vec = *static_cast<TVector3i*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z;
			}
			break;
		case ETypeKind::Vector4i:
			{
				TVector4i& vec = *static_cast<TVector4i*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z >> vec.W;
			}
			break;
		case ETypeKind::Vector2u:
			{
				TVector2u& vec = *static_cast<TVector2u*>(componentData);
				archive >> vec.X >> vec.Y;
			}
			break;
		case ETypeKind::Vector3u:
			{
				TVector3u& vec = *static_cast<TVector3u*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z;
			}
			break;
		case ETypeKind::Vector4u:
			{
				TVector4u& vec = *static_cast<TVector4u*>(componentData);
				archive >> vec.X >> vec.Y >> vec.Z >> vec.W;
			}
			break;
		case ETypeKind::Custom:
		default:
			break;
		}
	}

	void ReflectiveContainer::SetupComponentFunctions(TypeComponent& component) const
	{
		switch (component.Kind)
		{
		case ETypeKind::Bool:
			TypeComponent::SetupFunctionsForType<bool>(component);
			break;
		case ETypeKind::Int8:
			TypeComponent::SetupFunctionsForType<int8>(component);
			break;
		case ETypeKind::Int16:
			TypeComponent::SetupFunctionsForType<int16>(component);
			break;
		case ETypeKind::Int32:
			TypeComponent::SetupFunctionsForType<int32>(component);
			break;
		case ETypeKind::Int64:
			TypeComponent::SetupFunctionsForType<int64>(component);
			break;
		case ETypeKind::UInt8:
			TypeComponent::SetupFunctionsForType<uint8>(component);
			break;
		case ETypeKind::UInt16:
			TypeComponent::SetupFunctionsForType<uint16>(component);
			break;
		case ETypeKind::UInt32:
			TypeComponent::SetupFunctionsForType<uint32>(component);
			break;
		case ETypeKind::UInt64:
			TypeComponent::SetupFunctionsForType<uint64>(component);
			break;
		case ETypeKind::Float:
			TypeComponent::SetupFunctionsForType<float>(component);
			break;
		case ETypeKind::Double:
			TypeComponent::SetupFunctionsForType<double>(component);
			break;
		case ETypeKind::Vector2f:
			TypeComponent::SetupFunctionsForType<TVector2f>(component);
			break;
		case ETypeKind::Vector3f:
			TypeComponent::SetupFunctionsForType<TVector3f>(component);
			break;
		case ETypeKind::Vector4f:
			TypeComponent::SetupFunctionsForType<TVector4f>(component);
			break;
		case ETypeKind::Vector2d:
			TypeComponent::SetupFunctionsForType<TVector2d>(component);
			break;
		case ETypeKind::Vector3d:
			TypeComponent::SetupFunctionsForType<TVector3d>(component);
			break;
		case ETypeKind::Vector4d:
			TypeComponent::SetupFunctionsForType<TVector4d>(component);
			break;
		case ETypeKind::Vector2i:
			TypeComponent::SetupFunctionsForType<TVector2i>(component);
			break;
		case ETypeKind::Vector3i:
			TypeComponent::SetupFunctionsForType<TVector3i>(component);
			break;
		case ETypeKind::Vector4i:
			TypeComponent::SetupFunctionsForType<TVector4i>(component);
			break;
		case ETypeKind::Vector2u:
			TypeComponent::SetupFunctionsForType<TVector2u>(component);
			break;
		case ETypeKind::Vector3u:
			TypeComponent::SetupFunctionsForType<TVector3u>(component);
			break;
		case ETypeKind::Vector4u:
			TypeComponent::SetupFunctionsForType<TVector4u>(component);
			break;
		case ETypeKind::Custom:
		default:
			break;
		}
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