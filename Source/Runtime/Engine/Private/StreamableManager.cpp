#include "StreamableManager.h"
#include "ResourceModule.h"

namespace Thunder
{
	void StreamableManager::LoadAllAsync()
	{
		ResourceModule::ForAllResources([](const TGuid& guid, NameHandle name)
		{
			ResourceModule::LoadAsync(name);
		});
	}
}
