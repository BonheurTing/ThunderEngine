#pragma once
#include "BasicDefinition.h"
#include "Guid.h"
#include "NameHandle.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	class GameObject
	{
	public:
		GameObject(GameObject* inOuter = nullptr) : Outer(inOuter) {}
		virtual ~GameObject() = default;
		_NODISCARD_ GameObject* GetOuter() const { return Outer; }

		virtual void Serialize(MemoryWriter& archive) {}
		virtual void DeSerialize(MemoryReader& archive) {}

	private:
		GameObject* Outer { nullptr };
	};

	enum class ETempGameResourceReflective : uint32
	{
		StaticMesh = 0,
		Texture2D,
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
		void SetResourceName(const NameHandle& name) { ResourceName = name; }

		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;
		virtual void OnResourceLoaded()
		{
			LOG("success OnResourceLoaded");
		}

	private:
		TGuid Guid {};
		ETempGameResourceReflective ResourceType { ETempGameResourceReflective::Unknown }; // 资源类型
		NameHandle ResourceName {}; // 资源的虚拟路径（如 "/Game/Textures.Texture1"）
		TArray<TGuid> Dependencies {}; // 该资源需要的其他资源的guid，加载的时候需要一起加载
	};
}
