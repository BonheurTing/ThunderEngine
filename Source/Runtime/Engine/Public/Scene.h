#pragma once
#include "GameObject.h"
#include "Entity.h"
#include "Container.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
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
		OnSceneLoaded(T* inLoadItem) : LoadItem(inLoadItem) {}

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
		Scene(GameObject* inOuter = nullptr);
		virtual ~Scene();

		// Entity management
		Entity* CreateEntity(const NameHandle& entityName = NameHandle::Empty);
		void AddRootEntity(Entity* rootEntity);
		void RemoveRootEntity(Entity* rootEntity);
		const TArray<Entity*>& GetRootEntities() const { return RootEntities; }

		// JSON serialization for scene files (.tmap)
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		void DeserializeJson(const rapidjson::Value& jsonValue);

		// Scene persistence
		bool Save(const String& fileFullPath);

		// Synchronous scene loading
		bool LoadSync(const String& fileFullPath);

		// Asynchronous scene loading
		void LoadAsync(const String& fileFullPath);

		// Asynchronous resource streaming
		void StreamScene();

		// Called when all scene resources are loaded
		virtual void OnLoaded();

		// Renderer association
		void SetRenderer(IRenderer* inRenderer) { Renderer = inRenderer; }
		IRenderer* GetRenderer() const { return Renderer; }

		// Scene identification
		void SetSceneName(const NameHandle& inName) { SceneName = inName; }
		const NameHandle& GetSceneName() const { return SceneName; }

	private:
		// Collect all resource dependencies in the scene
		void CollectDependencies(TArray<TGuid>& outDependencies) const;

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

		void AddRenderer(IRenderer* inRenderer, Scene* inScene)
		{
			Renderers.push_back(inRenderer);
			Scenes.push_back(inScene);
		}

		void RemoveRenderer(IRenderer* inRenderer)
		{
			for (size_t i = 0; i < Renderers.size(); ++i)
			{
				if (Renderers[i] == inRenderer)
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
