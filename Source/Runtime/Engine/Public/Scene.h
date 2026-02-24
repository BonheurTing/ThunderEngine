#pragma once
#include "GameObject.h"
#include "Entity.h"
#include "Container.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Thunder
{
	class IRenderer;

	/**
	 * Scene represents a game level/map
	 * Contains a hierarchy of entities and manages scene serialization
	 */
	class Scene : public GameObject, public ITickable
	{
	public:
		ENGINE_API Scene(const TFunction<class IRenderer*()>& renderFactory, GameObject* inOuter = nullptr);
		ENGINE_API virtual ~Scene();

		// Entity management
		ENGINE_API Entity* CreateEntity(const NameHandle& entityName = NameHandle::Empty);
		ENGINE_API void AddRootEntity(Entity* rootEntity);
		ENGINE_API void RemoveRootEntity(Entity* rootEntity);
		ENGINE_API const TArray<Entity*>& GetRootEntities() const { return RootEntities; }

		// JSON serialization for scene files (.tmap)
		ENGINE_API void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		ENGINE_API void DeserializeJson(const rapidjson::Value& jsonValue);

		ENGINE_API bool Save(const String& fileFullPath);
		ENGINE_API bool LoadSync(const String& fileFullPath);
		ENGINE_API void LoadAsync(const String& fileFullPath);
		ENGINE_API void OnLoaded();
		ENGINE_API void Tick() override;

		// Renderer association
		ENGINE_API void SetRenderer(IRenderer* inRenderer) { Renderer = inRenderer; }
		ENGINE_API IRenderer* GetRenderer() const { return Renderer; }

		// Scene identification
		ENGINE_API void SetSceneName(const NameHandle& inName) { SceneName = inName; }
		ENGINE_API const NameHandle& GetSceneName() const { return SceneName; }

	private:
		/*// Asynchronous resource streaming
		void StreamScene();
		// Collect all resource dependencies in the scene
		void SimulateStreamingWithDistance(TList<IComponent*>& outDependencies) const;*/

		NameHandle SceneName;
		TArray<Entity*> RootEntities;
		IRenderer* Renderer { nullptr };
	};

	class BaseViewport
	{
	public:
		BaseViewport() = default;
		virtual ~BaseViewport();

		void AddScene(Scene* scene) { Scenes.push_back(scene); }
		const TArray<Scene*>& GetScenes() const { return Scenes; }
		void GetRenderers(TArray<IRenderer*>& renderers) const;
		void SetNext(BaseViewport* viewport) { NextViewport = viewport; }
		BaseViewport* GetNext() const { return NextViewport; }

	private:
		TArray<Scene*> Scenes;
		BaseViewport* NextViewport { nullptr };
	};
}
