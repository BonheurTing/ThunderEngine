#include "StreamableManager.h"
#include "ResourceModule.h"

namespace Thunder
{
	void StreamableManager::ForceLoadAllResources()
	{
		ResourceModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			ResourceModule::LoadSync(guid);
		});
	}

	void StreamableManager::LoadAllAsync()
	{
		ResourceModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			ResourceModule::LoadAsync(guid);
		});
	}
}
