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

	private:
		GameObject* Outer { nullptr };
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
}
