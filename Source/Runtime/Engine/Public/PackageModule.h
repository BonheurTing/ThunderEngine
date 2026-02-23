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
		TMap<TGuid, TArray<TGuid>> Dependencies; //GameResource - GameResourceDependencies

		TArray<TFunction<void()>> Callbacks;
		uint32 PendingDependencyCount { 0 };
		bool IsLoadCompletedAndWaitingForDependencies { false };  // LoadSync completed, but may be waiting for dependencies

		// For incremental import.
		int64 SrcFileLastWriteTime { 0 };
		int64 SrcFileSize { 0 };

		PackageEntry(const TGuid& inGuid, NameHandle inSoftPath) : Guid(inGuid), SoftPath(inSoftPath) {}

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
				Status |= static_cast<uint32>(EPackageStatus::Loaded);
			else
				Status &= ~static_cast<uint32>(EPackageStatus::Loaded);
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
		// editor fbx/png/tga -> uasset
#if WITH_EDITOR
		static bool Import(const String& srcPath, const String& destPath);
		static bool ReImport(const String& srcPath, const String& destPath, const TGuid& existingPakGuid);
		static bool ImportTmap(const String& srcPath, const String& destPath);
		void ImportAll(bool bForce = false);
#endif

		// runtime
		// load package or game resource
		static bool LoadSync(const TGuid& inGuid, bool bForce);
		static void LoadAsync(const TGuid& inGuid, TFunction<void()>&& inFunction);

		bool SavePackage(Package* package);

		static PackageEntry* AddPackageEntry(const TGuid& guid, const NameHandle& path);
		static void AddResourceToPackage(const TGuid& resGuid, const TGuid& pakGuid) { GetModule()->ResourceToPackage.emplace(resGuid, pakGuid); }
		static void ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function);

		// get resource
		static bool IsLoaded(const TGuid& inGuid);
		static GameResource* TryGetLoadedResource(const TGuid& inGuid);
#if WITH_EDITOR
		static GameResource* GetResource(NameHandle softPath);
		static TGuid GetGuidBySoftPath(NameHandle softPath);
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
		
#if WITH_EDITOR
		static void AddSoftPathToGuid(NameHandle softPath, const TGuid& inGuid) { GetModule()->SoftPathToGuidMap.emplace(softPath, inGuid); }
#endif
		
	private:
		void PackageAcquire(PackageEntry* entry);
		TSet<TGuid> GetPackageDependencies(const TGuid& pakGuid);
#if WITH_EDITOR
		void RegisterPackageSoftPathToGUID(Package* package);
#endif

	private:
		// loading assistance
		uint32 FrameMaxDelivering = 3;
		TMap<TGuid, PackageEntry*> PackageMap {};
		TDeque<PackageEntry*> AcquireRequests;
		TDeque<PackageEntry*> CompletionList;

#if WITH_EDITOR
		TMap<NameHandle, TGuid> SoftPathToGuidMap{}; // A { Soft-path -> GUID } lookup for all resources and packages.
#endif

		TMap<TGuid, TGuid> ResourceToPackage {}; // { ResourceGUID -> PackageGUID } lookup, PackageGUID -> PackageGUID is also contained.
	};
}

