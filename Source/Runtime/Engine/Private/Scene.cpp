#pragma optimize("", off)
#include "Scene.h"
#include "GameModule.h"
#include "ResourceModule.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Thunder
{

	
	// Scene implementation
	Scene::Scene(const TFunction<IRenderer*()>& renderFactory, GameObject* inOuter)
		: GameObject(inOuter)
	{
		Renderer = renderFactory();
	}

	Scene::~Scene()
	{
		GameModule::UnregisterTickable(this);
	}

	Entity* Scene::CreateEntity(const NameHandle& entityName)
	{
		Entity* newEntity = new Entity(this, this);
		newEntity->SetEntityName(entityName);
		return newEntity;
	}

	void Scene::AddRootEntity(Entity* rootEntity)
	{
		if (rootEntity)
		{
			RootEntities.push_back(rootEntity);
		}
	}

	void Scene::RemoveRootEntity(Entity* rootEntity)
	{
		auto it = std::find(RootEntities.begin(), RootEntities.end(), rootEntity);
		if (it != RootEntities.end())
		{
			RootEntities.erase(it);
		}
	}

	void Scene::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
	{
		writer.StartObject();

		// Serialize scene name
		writer.Key("SceneName");
		writer.String(SceneName.c_str());

		// Serialize root entities
		writer.Key("RootEntities");
		writer.StartArray();
		for (Entity* rootEntity : RootEntities)
		{
			rootEntity->SerializeJson(writer);
		}
		writer.EndArray();

		writer.EndObject();
	}

	void Scene::DeserializeJson(const rapidjson::Value& jsonValue)
	{
		if (!jsonValue.IsObject())
		{
			return;
		}

		// Deserialize scene name
		if (jsonValue.HasMember("SceneName") && jsonValue["SceneName"].IsString())
		{
			SceneName = NameHandle(jsonValue["SceneName"].GetString());
		}

		// Deserialize root entities
		if (jsonValue.HasMember("RootEntities") && jsonValue["RootEntities"].IsArray())
		{
			const rapidjson::Value& entitiesArray = jsonValue["RootEntities"];
			for (rapidjson::SizeType i = 0; i < entitiesArray.Size(); ++i)
			{
				Entity* newEntity = new Entity(this, this);
				newEntity->DeserializeJson(entitiesArray[i]);
				RootEntities.push_back(newEntity);
			}
		}
	}

	bool Scene::Save(const String& fileFullPath)
	{
		// Create JSON writer
		rapidjson::StringBuffer stringBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

		// Serialize scene to JSON
		SerializeJson(writer);

		// Convert JSON to binary data
		const char* jsonString = stringBuffer.GetString();
		size_t jsonSize = stringBuffer.GetSize();

		// Write to file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		TAssert(fileSystem != nullptr);
		const String tempPath = fileFullPath + ".tmp";
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(tempPath, false));
		const size_t ret = file->Write(jsonString, jsonSize);

		return ret == jsonSize && file->Rename(tempPath, fileFullPath);
	}

	bool Scene::LoadSync(const String& fileFullPath)
	{
		// Read file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		TAssert(fileSystem != nullptr);
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(fileFullPath, false));
		TAssert(file != nullptr);
		// Get file size and read all content
		const size_t fileSize = file->Size();
		if (fileSize == 0)
		{
			LOG("Scene file is empty: %s", fileFullPath.c_str());
			file->Close();
			return false;
		}

		// Allocate buffer and read file
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		const size_t bytesRead = file->Read(fileData, fileSize);
		file->Close();

		if (bytesRead != fileSize)
		{
			LOG("Failed to read scene file: %s", fileFullPath.c_str());
			TMemory::Free(fileData);
			return false;
		}

		// Convert to string (assuming JSON is text)
		String jsonString(static_cast<const char*>(fileData), fileSize);
		TMemory::Free(fileData);

		// Parse JSON
		rapidjson::Document document;
		document.Parse(jsonString.c_str());
		TAssertf(!document.HasParseError(), "Failed to parse scene JSON: %s", fileFullPath.c_str());

		// Deserialize scene from JSON
		DeserializeJson(document);

		// Schedule OnLoaded callback on game thread
		// For now, call directly (in production, use GameScheduler->Invoke)
		OnLoaded();

		LOG("Scene loaded from: %s", fileFullPath.c_str());
		return true;
	}

	void Scene::LoadAsync(const String& fileFullPath)
	{
		// Push loading task to async worker pool
		GAsyncWorkers->PushTask([this, fileFullPath]()
		{
			LoadSync(fileFullPath);
		});
	}

	void Scene::OnLoaded()
	{
		LOG("------------ Scene ended streaming: %s", SceneName.c_str());
		GameModule::RegisterTickable(this);
	}

	void Scene::Tick()
	{
		TVector3f cameraLocation { TVector3f(0.f, 0.f, 0.f) };
		for (Entity* rootEntity : RootEntities)
		{
			if (Math::Distance(cameraLocation, rootEntity->GetTransform()->GetPosition()) > 0.5f)
			{
				rootEntity->Load();
			}
		}
	}

	BaseViewport::~BaseViewport()
	{
		for (auto scene : Scenes)
		{
			delete scene;
		}
		Scenes.clear();
		/*String fullPath = FileModule::GetResourceContentRoot() + "Map/TestScene.tmap";
		bool ret = TestScene->Save(fullPath);
		TAssertf(ret, "Fail to save scene");*/
	}

	void BaseViewport::GetRenderers(TArray<IRenderer*>& renderers) const
	{
		for (auto scene : Scenes)
		{
			renderers.push_back(scene->GetRenderer());
		}
	}

	/*void Scene::SimulateStreamingWithDistance(TList<IComponent*>& outDependencies) const
	{
		TVector3f cameraLocation { TVector3f(0.f, 0.f, 0.f) };
		for (Entity* rootEntity : RootEntities)
		{
			if (Math::Distance(cameraLocation, rootEntity->GetTransform()->GetPosition()) > 0.5f)
			{
				 rootEntity->GetAllHierarchyComponents(outDependencies);
			}
		}
	}

	void Scene::StreamScene()
	{
		// Collect all resource dependencies
		TList<IComponent*> compList;
		SimulateStreamingWithDistance(compList);
		if (compList.empty())
		{
			OnLoaded();
			return;
		}

		if (StreamingEvent == nullptr)
		{
			StreamingEvent = new (TMemory::Malloc<TLoadEvent<Scene>>()) TLoadEvent(this);
		}
		
		uint32 compNum = static_cast<uint32>(compList.size());
		TArray<IComponent*> compTable(compList.begin(), compList.end());

		StreamingEvent->Promise(static_cast<int>(compNum));
		GAsyncWorkers->ParallelFor([compTable, compNum, event = StreamingEvent](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
		{
			for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
			{
				if (index < compNum)
				{
					compTable[index]->SyncLoad();
					event->Notify();
				}
			}
		}, compNum);
	}*/
}
