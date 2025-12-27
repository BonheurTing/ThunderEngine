#pragma once
#include <ranges>

#include "GameObject.h"
#include "Package.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	enum class EPackageStatus : uint32
	{
		Undefined = 0,
		Acquiring = 1 << 1,
		Loading = (1 << 2),
		Loaded = (1 << 3),
	};

	struct PackageEntry
	{
		uint32 Status { 0 };
		TGuid Guid;
		NameHandle SoftPath;
		TWeakObjectPtr<Package> Package { nullptr };
		TMap<TGuid, TArray<TGuid>> Dependencies; //GameResource - GameResourceDependences

		bool IsAcquiring() const
		{
			return (Status & static_cast<uint32>(EPackageStatus::Acquiring)) != 0;
		}
		bool IsLoading() const
		{
			return (Status & static_cast<uint32>(EPackageStatus::Loading)) != 0;
		}
		bool IsLoaded() const
		{
			return (Status & static_cast<uint32>(EPackageStatus::Loaded)) != 0;
		}
		void SetAcquiring(bool bAcquiring)
		{
			if (bAcquiring)
				Status |= static_cast<uint32>(EPackageStatus::Acquiring);
			else
				Status &= ~static_cast<uint32>(EPackageStatus::Acquiring);
		}
		void SetLoaded(bool bLoaded)
		{
			if (bLoaded)
				Status &= ~static_cast<uint32>(EPackageStatus::Loading);
			else
				Status |= static_cast<uint32>(EPackageStatus::Loading);
		}
		TSet<TGuid> GetResourceDependencies()
		{
			TSet<TGuid> result;
			for (auto val : Dependencies | std::views::values)
			{
				for (auto guid : val)
				{
					result.emplace(guid);
				}
			}
			return result;
		}
	};

	struct ResourceCompletion
	{
		// TWeakObjectPtr<GameResource> Resource;
		TGuid PackageGuid;
		TFunction<void()> Callback;
	};

	class ENGINE_API PackageModule : public IModule, public ITickable
	{
		DECLARE_MODULE(Package, PackageModule)

	public:
		void StartUp() override {}
		void ShutDown() override;
		void Tick() override;
		
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
#if WITH_EDITOR
		static bool ForceImport(const String& srcPath, const String& destPath);
		void ImportAll(bool bForce = true);
#endif

		// runtime
		// load package
		static bool LoadSync(const TGuid& guid, TArray<GameResource*> &outResources, bool bForce = false);
		static bool ForceLoadBySoftPath(const NameHandle& softPath);
		// load game resource
		static GameResource* LoadSync(const TGuid& guid, bool bForce = false);
		static void LoadAsync(const TGuid& guid);
		void LoadAsync(const TGuid& inGuid, TFunction<void()>&& inFunction);

		//static bool IsLoaded(const TGuid& guid) { return GetModule()->LoadedResources.contains(guid); }
		bool SavePackage(Package* package);
		void RegisterPackage(Package* package);

		static void AddResourcePathPair(const TGuid& guid, const NameHandle& path) { GetModule()->ResourcePathMap.emplace(guid, path); }
		static void ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function);

		// get resource
#if WITH_EDITOR
		static GameObject* GetResource(NameHandle softPath);
#endif
		template<typename T>
		static TArray<GameObject*> GetResourceByClass()
		{
			TArray<GameObject*> result;
			for (const auto& pakEntry : GetModule()->PackageMap | std::views::values)
			{
				if (pakEntry->IsLoaded())
				{
					if constexpr (std::is_same_v<T, Package>)
					{
						result.push_back(pakEntry->Package.Get());
					}
					for (auto resource : pakEntry->Package->GetPackageObjects())
					{
						if (T* typedResource = dynamic_cast<T*>(resource)) //todo
						{
							result.push_back(typedResource);
						}
					}
				}
			}
			return result;
		}

		
	private:
		static GameObject* TryGetLoadedResource(const TGuid& inGuid);
		void PackageAcquire(PackageEntry* entry);
		TSet<TGuid> GetPackageDependencies(const TGuid& inGuid);

	private:
		// loading assistance
		uint32 FrameMaxDelivering = 3;
		TMap<TGuid, PackageEntry*> PackageMap {}; // 启动时扫描content目录建立guid到路径虚拟路径映射, 创建PackageEntry，Package为null
		TDeque<PackageEntry*> AcquireRequests;
		TArray<ResourceCompletion*> CompletionList;

		// query
		//TMap<TGuid, GameObject*> LoadedResources {}; // 已加载的资源(Package/GameResource)，使用 TGuid 作为键
#if WITH_EDITOR
		TMap<NameHandle, TGuid> LoadedResourcesByPath {}; // 使用虚拟路径作为键, 便于开发者使用，editor only
#endif												// 导入资源和新建/生成时需要检查重名问题，package load原则上不用检查

		TMap<TGuid, NameHandle> ResourcePathMap {}; // Package的路径映射+Package中Object的路径映射，使用 TGuid 作为键；
													// 启动时扫描content目录建立guid到路径虚拟路径映射，world streaming等使用

		TMap<TGuid, TGuid> ResourceToPackage {}; // 启动时扫描content目录建立resourece guid到Package guid 的映射，辅助加载卸载功能使用

		/* 虚拟路径检查
		 * 1 导入时检查 (必然是editor)
		 * 2 新建资源和修改资源原则上需要检查(必然是editor)，目前还没这个逻辑，先不写
		 * 3 因为12，所以save时一定是unique
		 * 4 load因为是save是unique，所以也是unique
		 */
	};
}

