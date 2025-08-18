#pragma once
#include "Container.h"
#include "Platform.h"
#include <functional>

#include "BasicDefinition.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	enum class ETypeKind : uint8
	{
		Invalid = 0,
		Bool,
		Int8,
		Int16,
		Int32,
		Int64,
		UInt8,
		UInt16,
		UInt32,
		UInt64,
		Float,
		Double,
		Vector2f,
		Vector3f,
		Vector4f,
		Vector2d,
		Vector3d,
		Vector4d,
		Vector2i,
		Vector3i,
		Vector4i,
		Vector2u,
		Vector3u,
		Vector4u,
		Custom
	};

	struct TypeComponent
	{
		size_t Size;
		size_t Offset;
		String Name;
		ETypeKind Kind;
		
		// Function pointers for type operations
		std::function<void(void*)> Constructor;
		std::function<void(void*)> Destructor;
		std::function<void(void*, const void*)> CopyConstructor;
		std::function<void(void*, void*)> MoveConstructor;
		std::function<void(void*, const void*)> CopyAssign;
		std::function<void(void*, void*)> MoveAssign;

		TypeComponent() : Size(0), Offset(0), Kind(ETypeKind::Invalid) {}
		
		template<typename T>
		static TypeComponent Create(const String& inName, size_t inOffset);
	};

	class ReflectiveContainer : public RefCountedObject
	{
	public:
		ReflectiveContainer();
		~ReflectiveContainer();

		// Copy and move constructors/assignment
		ReflectiveContainer(const ReflectiveContainer& other);
		ReflectiveContainer(ReflectiveContainer&& other) noexcept;
		ReflectiveContainer& operator=(const ReflectiveContainer& other);
		ReflectiveContainer& operator=(ReflectiveContainer&& other) noexcept;

		// Add a component type to the container
		template<typename T>
		void AddComponent(const String& name);

		void SetDataNum(size_t num) { DataNum = num; }

		// Initialize the container with calculated size
		void Initialize();

		void CopyData(const void* src, size_t offset, size_t size) const;

		// Get/Set component values
		template<typename T>
		T* GetComponent(const String& name);
		
		template<typename T>
		const T* GetComponent(const String& name) const;

		template<typename T>
		void SetComponent(const String& name, const T& value);

		// Get component by index
		template<typename T>
		T* GetComponentByIndex(size_t index);
		
		template<typename T>
		const T* GetComponentByIndex(size_t index) const;

		// Utility functions
		_NODISCARD_ size_t GetComponentCount() const { return Components.size(); }
		_NODISCARD_ size_t GetStride() const { return Stride; }
		_NODISCARD_ size_t GetDataNum() const { return DataNum; }
		_NODISCARD_ size_t GetTotalSize() const { return Stride * DataNum; }
		_NODISCARD_ const TypeComponent* GetComponentInfo(const String& name) const;
		_NODISCARD_ const TypeComponent* GetComponentInfo(size_t index) const;
		_NODISCARD_ size_t GetComponentOffset(const String& name) const { return GetComponentInfo(name)->Offset; }
		
		// Data access
		//void* GetData() { return Data; }
		_NODISCARD_ const void* GetData() const { return Data; }

		// Clear all data
		void Clear();

	private:
		void* Data;
		TArray<TypeComponent> Components;
		size_t Stride;
		size_t DataNum;
		bool bInitialized;

		// Helper functions
		void CalculateLayout();
		void AllocateData();
		void DestroyData();
		void CopyData(const ReflectiveContainer& other);
		size_t FindComponentIndex(const String& name) const;
	};
	using TReflectiveContainerRef = TRefCountPtr<ReflectiveContainer>;
}

