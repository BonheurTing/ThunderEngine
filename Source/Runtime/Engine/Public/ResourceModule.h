#pragma once
#include "GameObject.h"
#include "Package.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	class ResourceModule : public IModule
	{
		DECLARE_MODULE(Resource, ResourceModule)
	public:
		static String CovertFullPathToContentPath(const String& fullPath);
		static String ConvertContentPathToFullPath(const String& contentPath, const String& extension = ".tasset");
		// 离线 fbx/png/tga -> uasset
		static bool Import(const String& srcPath, const String& destPath);
		static void ImportAll(bool bForce = true);

		// 运行时
		void LoadSync(const String& path);
		void LoadAsync(const String& path);
		bool IsLoaded(const String& path) const { return LoadedResourcesByPath.contains(path); }
		bool IsLoaded(const GUID& guid) const { return LoadedResources.contains(guid); }
		bool SavePackage(Package* package, const String& fullPath);

	private:
		TMap<GUID, GameResource*> LoadedResources; // 已加载的资源，使用 GUID 作为键
		TMap<String, GUID> LoadedResourcesByPath; // 使用虚拟路径作为键, 便于开发者使用，仅editor有

		TMap<GUID, String> ResourcePathMap; // Package的路径映射，使用 GUID 作为键；启动时扫描content目录建立guid到路径虚拟路径映射，world streaming等使用
	};
}

