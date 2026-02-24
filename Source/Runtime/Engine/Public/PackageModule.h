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

	class PackageModule : public IModule, public ITickable
	{
		DECLARE_MODULE(Package, PackageModule, ENGINE_API)

	public:
		ENGINE_API void StartUp() override {}
		ENGINE_API void ShutDown() override;
		ENGINE_API void Tick() override;
		
		ENGINE_API static void InitResourcePathMap(); // must after StartUp

		/**
		 * soft path:
		 * Absolute path : D:\ThunderEngine\Content\Meshes\FurnitureSet.tasset
		 * Soft path is relative : /Game/Meshes/FurnitureSet
		 * This package contains several GameResource, /Game/Meshes/FurnitureSet.Table /Game/Meshes/FurnitureSet.Chair, suffix is resource name
		 * Check if the name is unique when creating a package, if not, add postfix like "_1", "_2"
		 **/
		ENGINE_API static String CovertFullPathToSoftPath(const String& fullPath, const String& resourceName = "");
		ENGINE_API static String ConvertSoftPathToFullPath(const String& softPath, const String& extension = ".tasset");
		ENGINE_API static bool CheckUniqueSoftPath(String& softPath);
		// editor fbx/png/tga -> uasset
#if WITH_EDITOR
		ENGINE_API static bool Import(const String& srcPath, const String& destPath);
		ENGINE_API static bool ReImport(const String& srcPath, const String& destPath, const TGuid& existingPakGuid);
		ENGINE_API static bool ImportTmap(const String& srcPath, const String& destPath);
		ENGINE_API void ImportAll(bool bForce = false);
#endif

		// runtime
		// load package or game resource
		ENGINE_API static bool LoadSync(const TGuid& inGuid, bool bForce);
		ENGINE_API static void LoadAsync(const TGuid& inGuid, TFunction<void()>&& inFunction);

		ENGINE_API bool SavePackage(Package* package);

		ENGINE_API static PackageEntry* AddPackageEntry(const TGuid& guid, const NameHandle& path);
		ENGINE_API static void AddResourceToPackage(const TGuid& resGuid, const TGuid& pakGuid) { GetModule()->ResourceToPackage.emplace(resGuid, pakGuid); }
		ENGINE_API static void ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function);

		// get resource
		ENGINE_API static bool IsLoaded(const TGuid& inGuid);
		ENGINE_API static GameResource* TryGetLoadedResource(const TGuid& inGuid);
#if WITH_EDITOR
		ENGINE_API static GameResource* GetResource(NameHandle softPath);
		ENGINE_API static TGuid GetGuidBySoftPath(NameHandle softPath);
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
		ENGINE_API static void AddSoftPathToGuid(NameHandle softPath, const TGuid& inGuid) { GetModule()->SoftPathToGuidMap.emplace(softPath, inGuid); }
#endif
		
	private:
		ENGINE_API void PackageAcquire(PackageEntry* entry);
		ENGINE_API TSet<TGuid> GetPackageDependencies(const TGuid& pakGuid);
#if WITH_EDITOR
		ENGINE_API void RegisterPackageSoftPathToGUID(Package* package);
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

