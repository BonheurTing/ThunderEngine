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
	class ENGINE_API Scene : public GameObject, public ITickable
	{
	public:
		Scene(const TFunction<class IRenderer*()>& renderFactory, GameObject* inOuter = nullptr);
		virtual ~Scene();

		// Entity management
		Entity* CreateEntity(const NameHandle& entityName = NameHandle::Empty);
		void AddRootEntity(Entity* rootEntity);
		void RemoveRootEntity(Entity* rootEntity);
		const TArray<Entity*>& GetRootEntities() const { return RootEntities; }

		// JSON serialization for scene files (.tmap)
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		void DeserializeJson(const rapidjson::Value& jsonValue);

		bool Save(const String& fileFullPath);
		bool LoadSync(const String& fileFullPath);
		void LoadAsync(const String& fileFullPath);
		void OnLoaded();
		void Tick() override;

		// Renderer association
		void SetRenderer(IRenderer* inRenderer) { Renderer = inRenderer; }
		IRenderer* GetRenderer() const { return Renderer; }

		// Scene identification
		void SetSceneName(const NameHandle& inName) { SceneName = inName; }
		const NameHandle& GetSceneName() const { return SceneName; }

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
