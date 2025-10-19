#pragma once
#include "GameObject.h"
#include "Package.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	class ENGINE_API ResourceModule : public IModule
	{
		DECLARE_MODULE(Resource, ResourceModule)

	public:
		void StartUp() override {}
		void ShutDown() override {}
		static void InitResourcePathMap(); // must after StartUp

		/**
		 * soft path命名规则：
		 * 磁盘路径是绝对路径，D:\ThunderEngine\Content\Meshes\FurnitureSet.tasset
		 * 软路径是相对路径，/Game/Meshes/FurnitureSet 是Package的虚拟路径
		 * 这个文件中有几个GameResource, /Game/Meshes/FurnitureSet.Table /Game/Meshes/FurnitureSet.Chair 后缀名是resource name
		 * 入包时检查是否重名，如重名则加_1_2
		 **/
		static String CovertFullPathToSoftPath(const String& fullPath, const String& resourceName = "");
		static String ConvertSoftPathToFullPath(const String& softPath, const String& extension = ".tasset");
		static bool CheckUniqueSoftPath(String& softPath);
		// 离线 fbx/png/tga -> uasset
		static bool Import(const String& srcPath, const String& destPath);
		void ImportAll(bool bForce = true);

		// runtime
		// load package
		static bool LoadSync(const TGuid& guid, TArray<GameResource*> &outResources, bool bForce = false);
		// load game resource
		static GameResource* LoadSync(const TGuid& guid, bool bForce = false);
		static void LoadAsync(const TGuid& guid);

		static bool IsLoaded(const TGuid& guid) { return GetModule()->LoadedResources.contains(guid); }
		bool SavePackage(Package* package, const String& fullPath);
		void RegisterPackage(Package* package);

		static void AddResourcePathPair(const TGuid& guid, const NameHandle& path) { GetModule()->ResourcePathMap.emplace(guid, path); }
		static void ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function);

		// get resource
		template<typename T>
		static TArray<GameObject*> GetResourceByClass()
		{
			TArray<GameObject*> result;
			for (const auto& [guid, resource] : GetModule()->LoadedResources)
			{
				if (T* typedResource = dynamic_cast<T*>(resource)) //todo
				{
					result.push_back(typedResource);
				}
			}
			return result;
		}
	private:
		static GameObject* TryGetLoadedResource(const TGuid& guid);
		static bool ForceLoadBySoftPath(const NameHandle& softPath);

	private:
		TMap<TGuid, GameObject*> LoadedResources {}; // 已加载的资源(Package/GameResource)，使用 TGuid 作为键
#ifdef WITH_EDITOR
		TMap<NameHandle, TGuid> LoadedResourcesByPath {}; // 使用虚拟路径作为键, 便于开发者使用，editor only
#endif												// 导入资源和新建/生成时需要检查重名问题，package load原则上不用检查

		TMap<TGuid, NameHandle> ResourcePathMap {}; // Package的路径映射+Package中Object的路径映射，使用 TGuid 作为键；
													// 启动时扫描content目录建立guid到路径虚拟路径映射，world streaming等使用

		/* 虚拟路径检查
		 * 1 导入时检查 (必然是editor)
		 * 2 新建资源和修改资源原则上需要检查(必然是editor)，目前还没这个逻辑，先不写
		 * 3 因为12，所以save时一定是unique
		 * 4 load因为是save是unique，所以也是unique
		 */
	};
}

