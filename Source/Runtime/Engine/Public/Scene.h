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
