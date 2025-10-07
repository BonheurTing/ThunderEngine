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
		void StartUp() override;
		void ShutDown() override {}

		/**
		 * soft path命名规则：
		 * 磁盘路径是绝对路径，D:\ThunderEngine\Content\Meshes\FurnitureSet.tasset
		 * 软路径是相对路径，/Game/Meshes/FurnitureSet 是Package的虚拟路径
		 * 这个文件中有几个GameResource, /Game/Meshes/FurnitureSet.Table /Game/Meshes/FurnitureSet.Chair 后缀名是resource name
		 **/
		static String CovertFullPathToSoftPath(const String& fullPath, const String& resourceName = "");
		static String ConvertSoftPathToFullPath(const String& softPath, const String& extension = ".tasset");
		// 离线 fbx/png/tga -> uasset
		static bool Import(const String& srcPath, const String& destPath);
		void ImportAll(bool bForce = true);

		// 运行时
		// load package
		static bool LoadSync(const NameHandle& softPath, TArray<GameResource*> &outResources, bool bForce = false);
		// load game resource
		static GameResource* LoadSync(const TGuid& guid, bool bForce = false);
		static GameResource* LoadSync(const NameHandle& resourceSoftPath, bool bForce = false);
		static void LoadAsync(const TGuid& guid);
		static void LoadAsync(const NameHandle& path);
		static bool IsLoaded(const NameHandle& path) { return GetModule()->LoadedResourcesByPath.contains(path); }
		static bool IsLoaded(const TGuid& guid) { return GetModule()->LoadedResources.contains(guid); }
		bool SavePackage(Package* package, const String& fullPath);
		void RegisterPackage(Package* package);

		_NODISCARD_ TMap<TGuid, NameHandle> const& GetResourcePathMap() const { return ResourcePathMap; }
		static void ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& Function);

	private:
		TMap<TGuid, GameObject*> LoadedResources {}; // 已加载的资源(Package/GameResource)，使用 TGuid 作为键
		TMap<NameHandle, TGuid> LoadedResourcesByPath {}; // 使用虚拟路径作为键, 便于开发者使用，仅editor有

		TMap<TGuid, NameHandle> ResourcePathMap {}; // Package的路径映射，使用 TGuid 作为键；启动时扫描content目录建立guid到路径虚拟路径映射，world streaming等使用
	};
}

