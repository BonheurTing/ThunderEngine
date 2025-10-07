#include "Scene.h"
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
	Scene::Scene(GameObject* InOuter)
		: GameObject(InOuter)
	{
	}

	Scene::~Scene()
	{
		for (Entity* RootEntity : RootEntities)
		{
			delete RootEntity;
		}
		RootEntities.clear();
	}

	Entity* Scene::CreateEntity(const NameHandle& EntityName)
	{
		Entity* NewEntity = new Entity(this);
		NewEntity->SetEntityName(EntityName);
		return NewEntity;
	}

	void Scene::AddRootEntity(Entity* RootEntity)
	{
		if (RootEntity)
		{
			RootEntities.push_back(RootEntity);
		}
	}

	void Scene::RemoveRootEntity(Entity* RootEntity)
	{
		auto It = std::find(RootEntities.begin(), RootEntities.end(), RootEntity);
		if (It != RootEntities.end())
		{
			RootEntities.erase(It);
		}
	}

	void Scene::SerializeJson(rapidjson::Writer<rapidjson::StringBuffer>& Writer) const
	{
		Writer.StartObject();

		// Serialize scene name
		Writer.Key("SceneName");
		Writer.String(SceneName.c_str());

		// Serialize root entities
		Writer.Key("RootEntities");
		Writer.StartArray();
		for (Entity* RootEntity : RootEntities)
		{
			RootEntity->SerializeJson(Writer);
		}
		Writer.EndArray();

		Writer.EndObject();
	}

	void Scene::DeserializeJson(const rapidjson::Value& JsonValue)
	{
		if (!JsonValue.IsObject())
		{
			return;
		}

		// Deserialize scene name
		if (JsonValue.HasMember("SceneName") && JsonValue["SceneName"].IsString())
		{
			SceneName = NameHandle(JsonValue["SceneName"].GetString());
		}

		// Deserialize root entities
		if (JsonValue.HasMember("RootEntities") && JsonValue["RootEntities"].IsArray())
		{
			const rapidjson::Value& EntitiesArray = JsonValue["RootEntities"];
			for (rapidjson::SizeType i = 0; i < EntitiesArray.Size(); ++i)
			{
				Entity* NewEntity = new Entity(this);
				NewEntity->DeserializeJson(EntitiesArray[i]);
				RootEntities.push_back(NewEntity);
			}
		}
	}

	bool Scene::Save(const String& FilePath)
	{
		// Create JSON writer
		rapidjson::StringBuffer StringBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer(StringBuffer);

		// Serialize scene to JSON
		SerializeJson(Writer);

		// Convert JSON to binary data
		const char* JsonString = StringBuffer.GetString();
		size_t JsonLength = StringBuffer.GetSize();

		// Write to file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		const String tempPath = FilePath + ".tmp";
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(tempPath, false));
		const size_t ret = file->Write(JsonString, JsonLength);

		file->Close();

		if (ret != JsonLength)
		{
			LOG("Failed to write scene data to file: %s", FilePath.c_str());
			return false;
		}

		return file->Rename(tempPath, FilePath);
	}

	bool Scene::LoadSync(const String& FilePath)
	{
		// Read file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			LOG("Failed to get file system");
			return false;
		}

		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(FilePath, false));
		if (!file)
		{
			LOG("Failed to open scene file: %s", FilePath.c_str());
			return false;
		}

		// Get file size and read all content
		const size_t fileSize = file->Size();
		if (fileSize == 0)
		{
			LOG("Scene file is empty: %s", FilePath.c_str());
			file->Close();
			return false;
		}

		// Allocate buffer and read file
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		const size_t bytesRead = file->Read(fileData, fileSize);
		file->Close();

		if (bytesRead != fileSize)
		{
			LOG("Failed to read scene file: %s", FilePath.c_str());
			TMemory::Free(fileData);
			return false;
		}

		// Convert to string (assuming JSON is text)
		String JsonString(static_cast<const char*>(fileData), fileSize);
		TMemory::Free(fileData);

		// Parse JSON
		rapidjson::Document Document;
		Document.Parse(JsonString.c_str());

		if (Document.HasParseError())
		{
			LOG("Failed to parse scene JSON: %s", FilePath.c_str());
			return false;
		}

		// Deserialize scene from JSON
		DeserializeJson(Document);

		// Schedule OnLoaded callback on game thread
		// For now, call directly (in production, use GameScheduler->Invoke)
		OnLoaded();

		LOG("Scene loaded synchronously from: %s", FilePath.c_str());
		return true;
	}

	void Scene::LoadAsync(const String& FilePath)
	{
		// Push loading task to async worker pool
		GAsyncWorkers->PushTask([this, FilePath]()
		{
			LoadSync(FilePath);
		});
	}

	bool Scene::Load(const String& FilePath)
	{
		// Read file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			LOG("Failed to get file system");
			return false;
		}

		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(FilePath, false));
		if (!file)
		{
			LOG("Failed to open scene file: %s", FilePath.c_str());
			return false;
		}

		// Get file size and read all content
		const size_t fileSize = file->Size();
		if (fileSize == 0)
		{
			LOG("Scene file is empty: %s", FilePath.c_str());
			file->Close();
			return false;
		}

		// Allocate buffer and read file
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		const size_t bytesRead = file->Read(fileData, fileSize);
		file->Close();

		if (bytesRead != fileSize)
		{
			LOG("Failed to read scene file: %s", FilePath.c_str());
			TMemory::Free(fileData);
			return false;
		}

		// Convert to string (assuming JSON is text)
		String JsonString(static_cast<const char*>(fileData), fileSize);
		TMemory::Free(fileData);

		// Parse JSON
		rapidjson::Document Document;
		Document.Parse(JsonString.c_str());

		if (Document.HasParseError())
		{
			LOG("Failed to parse scene JSON: %s", FilePath.c_str());
			return false;
		}

		// Deserialize scene from JSON
		DeserializeJson(Document);

		LOG("Scene loaded successfully from: %s", FilePath.c_str());
		return true;
	}

	void Scene::CollectDependencies(TArray<TGuid>& OutDependencies) const
	{
		for (Entity* RootEntity : RootEntities)
		{
			RootEntity->GetDependencies(OutDependencies);
		}
	}

	void Scene::StreamScene()
	{
		// Collect all resource dependencies
		TArray<TGuid> GuidList;
		CollectDependencies(GuidList);
		uint32 guidNum = static_cast<uint32>(GuidList.size());

		if (GuidList.empty())
		{
			// No resources to load, call OnLoaded directly
			OnLoaded();
			return;
		}

		auto* callback = new (TMemory::Malloc<OnSceneLoaded<Scene>>()) OnSceneLoaded(this);
		callback->Promise(guidNum);
		uint32 taskBundleSize = guidNum / GAsyncWorkers->GetNumThreads();
		GAsyncWorkers->ParallelFor([GuidList, callback](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
		{
			for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
			{
				auto guid = GuidList[index];
				ResourceModule::LoadSync(guid);
				callback->Notify();
			}
		}, guidNum, taskBundleSize);
		TMemory::Destroy(callback);
	}

	void Scene::OnLoaded()
	{
		LOG("Scene Loaded: %s", SceneName.c_str());
	}
}
