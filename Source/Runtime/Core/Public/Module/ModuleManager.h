#pragma once
#include "NameHandle.h"
#include "Platform.h"
#include "RefCount.h"

#define DEFINE_MODULE(ModuleName, ModuleType) \
	static ModuleRegistry<ModuleType> ModuleName##Registry;

#define IMPLEMENT_MODULE(ModuleName, ModuleType) \
	ModuleRegistry<ModuleType> ModuleType::ModuleName##Registry{#ModuleName};

namespace Thunder
{
	class IModule
	{
	public:
		virtual ~IModule() = default;
		virtual void StartUp() {}
		virtual void ShutDown() {}
	
	private:
		NameHandle Name;
	};

	class ModuleManager
	{
	public:
		static ModuleManager* GetInstance()
		{
			static auto inst = new ModuleManager();
			return inst;
		}
		
		void RegisterModule(NameHandle name, IModule* module);
		void LoadModuleByName(NameHandle name);
		void UnloadModuleByName(NameHandle name);
		
	private:
		ModuleManager() = default;
		HashMap<NameHandle, RefCountPtr<IModule>> ModuleMap;
	};

	template<typename ModuleType>
	struct ModuleRegistry
	{
		ModuleRegistry(NameHandle name)
		{
			auto const module = ModuleType::GetInstance();
			ModuleManager::GetInstance()->RegisterModule(name, module);
		}
	};
}
