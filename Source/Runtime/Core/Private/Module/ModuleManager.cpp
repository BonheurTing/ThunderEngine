#include "Module/ModuleManager.h"
#include <memory>
#include "Assertion.h"

namespace Thunder
{
	Mutex ModuleManager::InstanceMutex{};
	ModuleManager* ModuleManager::Instance = nullptr;
	void ModuleManager::RegisterModule(NameHandle name, Function<IModule*()>& registerFunc)
	{
		TAssertf(!ModuleRegisterMap.contains(name), "Module \"%s\" already registerd", name.c_str());
		ModuleRegisterMap[name] = std::move(registerFunc);
		LOG("Module registered : \"%s\"", name.c_str());
	}

	bool ModuleManager::IsModuleRegistered(NameHandle name) const
	{
		return ModuleRegisterMap.contains(name);
	}

	IModule* ModuleManager::GetModuleByName(NameHandle name)
	{
		if (ModuleMap.contains(name))
		{
			return ModuleMap[name].get();
		}
		TAssertf(false, "Module \"%s\" doesn't exist.", name.c_str());
		return nullptr;
	}

	void ModuleManager::InternalLoadModuleByName(NameHandle name)
	{
		if (ModuleMap.contains(name))
		{
			TAssertf(false, "Module \"%s\" already exists.", name.c_str());
		}
		else if (ModuleRegisterMap.contains(name))
		{
			auto const moduleInst = ModuleRegisterMap[name].operator()();
			ModuleMap[name] = RefCountPtr<IModule>(moduleInst);
			moduleInst->StartUp();
			LOG("Module \"%s\" loaded.", name.c_str());
		}
		else
		{
			TAssertf(false, "Module \"%s\" hasn't been registered yet.", name.c_str());
		}
	}

	void ModuleManager::InternalUnloadModuleByName(NameHandle name)
	{
		if (ModuleMap.contains(name))
		{
			ModuleMap[name]->ShutDown();
		}
		else
		{
			TAssertf(false, "Module \"%s\" doesn't exist", name.c_str());
		}
	}
}
