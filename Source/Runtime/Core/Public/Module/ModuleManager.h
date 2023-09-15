#pragma once
#include "BasicDefinition.h"
#include "Callable.h"
#include "NameHandle.h"
#include "RefCount.h"

#define DECLARE_MODULE_CTOR(ModuleName, ModuleType) \
	public: \
		ModuleType() : IModule(#ModuleName) {}

#define DECLARE_MODULE_CTOR_SUPER(ModuleName, ModuleType, SuperType) \
	public: \
		ModuleType() : SuperType(#ModuleName) {}

#define DECLARE_MODULE_COMMON(ModuleName, ModuleType) \
	public: \
		static NameHandle GetStaticName() { return StaticName; } \
	private: \
		static NameHandle StaticName; \
		static ModuleRegistry<ModuleType> ModuleName##Registry;

#define DECLARE_MODULE(ModuleName, ModuleType) \
	DECLARE_MODULE_CTOR(ModuleName, ModuleType) \
	DECLARE_MODULE_COMMON(ModuleName, ModuleType)

#define DECLARE_MODULE_WITH_SUPER(ModuleName, ModuleType, SuperType) \
	DECLARE_MODULE_CTOR_SUPER(ModuleName, ModuleType, SuperType) \
	DECLARE_MODULE_COMMON(ModuleName, ModuleType)

#define IMPLEMENT_MODULE(ModuleName, ModuleType) \
	NameHandle ModuleType::StaticName = #ModuleName; \
	Function ModuleName##RegisterFunction = ModuleFactory<ModuleType>{}; \
	ModuleRegistry<ModuleType> ModuleType::ModuleName##Registry{#ModuleName, ModuleName##RegisterFunction}; \


namespace Thunder
{
	class CORE_API IModule
	{
	public:
		IModule(NameHandle name) : Name(name) {}
		virtual ~IModule() = default;
		virtual void StartUp() = 0;
		virtual void ShutDown() = 0;
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
			if (Instance == nullptr)
			{
				ScopeLock lock(InstanceMutex);
				if (Instance == nullptr)
				{
					Instance = new ModuleManager();
				}
			}
			return Instance;
		}
		static void DestroyInstance()
		{
			if (Instance)
			{
				ScopeLock lock(InstanceMutex);
				if (Instance)
				{
					delete Instance;
					Instance = nullptr;
				}
			}
		}

		void RegisterModule(NameHandle name, Function<IModule*(void)>& registerFunc);
		bool IsModuleRegistered(NameHandle name) const;
		_NODISCARD_ IModule* GetModuleByName(NameHandle name);
		void LoadModuleByName(NameHandle name);
		void UnloadModuleByName(NameHandle name);

		template<typename ModuleType>
		void LoadModule()
		{
			LoadModuleByName(ModuleType::GetStaticName());
		}

		template<typename ModuleType>
		void UnloadModule()
		{
			UnloadModuleByName(ModuleType::GetStaticName());
		}

	private:
		ModuleManager() = default;
		static Mutex InstanceMutex;
		static ModuleManager* Instance;
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
