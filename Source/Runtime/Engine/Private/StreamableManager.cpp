#include "StreamableManager.h"
#include "PackageModule.h"

namespace Thunder
{
	void StreamableManager::ForceLoadAllResources()
	{
		PackageModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			PackageModule::LoadSync(guid, false);
		});
	}

	void StreamableManager::LoadAllAsync()
	{
		PackageModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			PackageModule::GetModule()->LoadAsync(guid, nullptr);
		});
	}
}
