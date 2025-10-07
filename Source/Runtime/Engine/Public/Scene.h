#pragma once
#include "GameObject.h"
#include "Entity.h"
#include "Container.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/ConcurrentBase.h"

namespace Thunder
{
	class IRenderer;

	/**
	 * Generic load event callback template
	 * Used for notifying when asynchronous resource loading is complete
	 */
	template <typename T>
	class OnSceneLoaded : public IOnCompleted
	{
	public:
		OnSceneLoaded(T* InLoadItem) : LoadItem(InLoadItem) {}

		void OnCompleted() override
		{
			if (LoadItem)
			{
				GGameScheduler->PushTask([item = LoadItem]()
				{
					item->OnLoaded();
				});
			}
		}

	private:
		T* LoadItem { nullptr };
	};

	/**
	 * Scene represents a game level/map
	 * Contains a hierarchy of entities and manages scene serialization
	 */
	class ENGINE_API Scene : public GameObject
	{
	public:
		Scene(GameObject* InOuter = nullptr);
		virtual ~Scene();

		// Entity management
		Entity* CreateEntity(const NameHandle& EntityName = NameHandle::Empty);
		void AddRootEntity(Entity* RootEntity);
		void RemoveRootEntity(Entity* RootEntity);
		const TArray<Entity*>& GetRootEntities() const { return RootEntities; }

		// JSON serialization for scene files (.tmap)
		void SerializeJson(rapidjson::Writer<rapidjson::StringBuffer>& Writer) const;
		void DeserializeJson(const rapidjson::Value& JsonValue);

		// Scene persistence
		bool Save(const String& FilePath);
		bool Load(const String& FilePath);

		// Synchronous scene loading
		bool LoadSync(const String& FilePath);

		// Asynchronous scene loading
		void LoadAsync(const String& FilePath);

		// Asynchronous resource streaming
		void StreamScene();

		// Called when all scene resources are loaded
		virtual void OnLoaded();

		// Renderer association
		void SetRenderer(IRenderer* InRenderer) { Renderer = InRenderer; }
		IRenderer* GetRenderer() const { return Renderer; }

		// Scene identification
		void SetSceneName(const NameHandle& InName) { SceneName = InName; }
		const NameHandle& GetSceneName() const { return SceneName; }

	private:
		// Collect all resource dependencies in the scene
		void CollectDependencies(TArray<TGuid>& OutDependencies) const;

		NameHandle SceneName;
		TArray<Entity*> RootEntities;
		IRenderer* Renderer { nullptr };
	};

	/**
	 * Base viewport class that manages renderer and scene association
	 */
	class BaseViewport
	{
	public:
		BaseViewport() = default;
		virtual ~BaseViewport() = default;

		void AddRenderer(IRenderer* InRenderer, Scene* InScene)
		{
			Renderers.push_back(InRenderer);
			Scenes.push_back(InScene);
		}

		void RemoveRenderer(IRenderer* InRenderer)
		{
			for (size_t i = 0; i < Renderers.size(); ++i)
			{
				if (Renderers[i] == InRenderer)
				{
					Renderers.erase(Renderers.begin() + i);
					Scenes.erase(Scenes.begin() + i);
					break;
				}
			}
		}

		const TArray<IRenderer*>& GetRenderers() const { return Renderers; }
		const TArray<Scene*>& GetScenes() const { return Scenes; }

	private:
		// Renderers and Scenes must have the same size and correspond to each other
		TArray<IRenderer*> Renderers;
		TArray<Scene*> Scenes;
	};
}
