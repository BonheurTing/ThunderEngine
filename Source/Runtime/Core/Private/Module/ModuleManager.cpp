#include "Module/ModuleManager.h"

#include <memory>

#include "Assertion.h"

namespace Thunder
{
	void ModuleManager::RegisterModule(NameHandle name, IModule* module)
	{
		TAssertf(!ModuleMap.contains(name), "Module %s already registerd", name.c_str());
		ModuleMap[name] = RefCountPtr<IModule>(module);
		LOG("Module registered : \"%s\"\n", name.c_str());
	}

	void ModuleManager::LoadModuleByName(NameHandle name)
	{
		if (ModuleMap.contains(name))
		{
			ModuleMap[name]->StartUp();
		}
		TAssertf(false, "Module %s doesn't exist", name.c_str());
	}

	void ModuleManager::UnloadModuleByName(NameHandle name)
	{
		if (ModuleMap.contains(name))
		{
			ModuleMap[name]->ShutDown();
		}
		TAssertf(false, "Module %s doesn't exist", name.c_str());
	}
}
