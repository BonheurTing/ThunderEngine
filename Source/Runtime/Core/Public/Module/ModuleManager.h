#pragma once
#include "BasicDefinition.h"
#include "Callable.h"
#include "NameHandle.h"
#include "RefCount.h"

#define DECLARE_MODULE(ModuleName, ModuleType) \
	public: \
		ModuleType() : IModule(#ModuleName) {} \
		static NameHandle GetStaticName() { return StaticName; } \
	private: \
		static NameHandle StaticName; \
		static ModuleRegistry<ModuleType> ModuleName##Registry;

#define IMPLEMENT_MODULE(ModuleName, ModuleType) \
	NameHandle ModuleType::StaticName = #ModuleName; \
	Function ModuleName##RegisterFunction = ModuleFactory<ModuleType>{}; \
	ModuleRegistry<ModuleType> ModuleType::ModuleName##Registry{#ModuleName, ModuleName##RegisterFunction};

namespace Thunder
{
	class CORE_API IModule
	{
	public:
		IModule(NameHandle name) : Name(name) {}
		virtual ~IModule() = default;
		virtual void StartUp() {}
		virtual void ShutDown() {}
		_NODISCARD_ NameHandle GetName() const { return Name; }
	
	protected:
		NameHandle Name;
	};

	template<class ModuleType>
	struct ModuleFactory
	{
		IModule* operator()() const
		{
			return new ModuleType();
		}
	};

	class CORE_API ModuleManager
	{
	public:
		static ModuleManager* GetInstance()
		{
			static auto instance = new ModuleManager;
			return instance;
		}

		void RegisterModule(NameHandle name, Function<IModule*(void)>& registerFunc);
		_NODISCARD_ IModule* GetModuleByName(NameHandle name);
		void LoadModuleByName(NameHandle name);
		void UnloadModuleByName(NameHandle name);

	private:
		ModuleManager() = default;
		HashMap<NameHandle, RefCountPtr<IModule>> ModuleMap;
		HashMap<NameHandle, Function<IModule*()>> ModuleRegisterMap;
	};

	template<typename ModuleType>
	struct ModuleRegistry
	{
		ModuleRegistry(NameHandle name, Function<IModule*()>& registerType)
		{
			ModuleManager::GetInstance()->RegisterModule(name, registerType);
		}
	};
}
