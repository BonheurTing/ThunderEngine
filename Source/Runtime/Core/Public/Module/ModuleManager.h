#pragma once
#include "BasicDefinition.h"
#include "Callable.h"
#include "NameHandle.h"
#include "Templates/RefCounting.h"

#define DECLARE_MODULE_CTOR(ModuleName, ModuleType) \
	public: \
		ModuleType() : IModule(#ModuleName) {}

#define DECLARE_MODULE_CTOR_SUPER(ModuleName, ModuleType, SuperType) \
	public: \
		ModuleType() : SuperType(#ModuleName) {}

#define DECLARE_MODULE_COMMON(ModuleName, ModuleType, ModuleAPI) \
		friend ModuleManager; \
	public: \
		FORCEINLINE static NameHandle GetStaticName() { return StaticName; } \
		FORCEINLINE static ModuleType* GetModule() { return static_cast<ModuleType*>(ModuleInstance); } \
	private: \
		ModuleAPI static ModuleType* ModuleInstance; \
		ModuleAPI static NameHandle StaticName; \
		static ModuleRegistry<ModuleType> ModuleName##Registry;

#define DECLARE_MODULE(ModuleName, ModuleType, ModuleAPI) \
	DECLARE_MODULE_CTOR(ModuleName, ModuleType) \
	DECLARE_MODULE_COMMON(ModuleName, ModuleType, ModuleAPI)

#define DECLARE_MODULE_WITH_SUPER(ModuleName, ModuleType, SuperType, ModuleAPI) \
	DECLARE_MODULE_CTOR_SUPER(ModuleName, ModuleType, SuperType) \
	DECLARE_MODULE_COMMON(ModuleName, ModuleType, ModuleAPI)

#define IMPLEMENT_MODULE(ModuleName, ModuleType) \
	NameHandle ModuleType::StaticName = #ModuleName; \
	ModuleType* ModuleType::ModuleInstance = nullptr; \
	Function ModuleName##RegisterFunction = ModuleFactory<ModuleType>{}; \
	ModuleRegistry<ModuleType> ModuleType::ModuleName##Registry{#ModuleName, ModuleName##RegisterFunction}; \


namespace Thunder
{
	class IModule : public RefCountedObject
	{
		friend class ModuleManager;

	public:
		CORE_API IModule(NameHandle name) : Name(name) {}
		CORE_API virtual ~IModule() = default;
		CORE_API virtual void StartUp() = 0;
		CORE_API virtual void ShutDown() = 0;
		//virtual IModule* GetModule() = 0;
		CORE_API _NODISCARD_ NameHandle GetName() const { return Name; }

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

		template<typename ModuleType>
		void LoadModule()
		{
			InternalLoadModuleByName(ModuleType::GetStaticName());
			ModuleType::ModuleInstance = static_cast<ModuleType*>(ModuleMap[ModuleType::GetStaticName()].Get());
		}

		template<typename ModuleType>
		void UnloadModule()
		{
			InternalUnloadModuleByName(ModuleType::GetStaticName());
		}

	private:
		void InternalLoadModuleByName(NameHandle name);
		void InternalUnloadModuleByName(NameHandle name);

	private:
		ModuleManager() = default;
		static Mutex InstanceMutex;
		static ModuleManager* Instance;
		THashMap<NameHandle, TRefCountPtr<IModule>> ModuleMap;
		THashMap<NameHandle, Function<IModule*()>> ModuleRegisterMap;
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
