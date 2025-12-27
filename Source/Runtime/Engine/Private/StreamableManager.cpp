#include "StreamableManager.h"
#include "PackageModule.h"

namespace Thunder
{
	void StreamableManager::ForceLoadAllResources()
	{
		PackageModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			PackageModule::LoadSync(guid);
		});
	}

	void StreamableManager::LoadAllAsync()
	{
		PackageModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			PackageModule::LoadAsync(guid);
		});
	}
}
