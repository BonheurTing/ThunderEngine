#pragma once

#include "Assertion.h"


/** Allows inline friend declaration without forward-declaring TLazySingleton */
class FLazySingleton
{
protected:
	template<class T> static void Construct(void* Place)	{ ::new (Place) T; }
	template<class T> static void Destruct(T* Instance)		{ Instance->~T(); }
};

/**
 * Lazy singleton that can be torn down explicitly
 * 
 * Defining DISABLE_LAZY_SINGLETON_DESTRUCTION stops automatic static destruction
 * and will instead leak singletons that have not been explicitly torn down.
 * This helps prevent shutdown crashes and deadlocks in quick exit scenarios
 * such as ExitProcess() on Windows.
 *
 * T must be default constructible.
 *
 * Example use case:
 *
 * struct FOo
 * 	{
 * 	static FOo& Get();
 * 	static void TearDown();
 * 
 * 	// If default constructor is private
 * 	friend class FLazySingleton;
 * };
 * 
 * // Only include in .cpp and do *not* inline Get() and TearDown()
 * #include "Misc/LazySingleton.h"
 * 
 * FOo& FOo::Get()
 * {
 * 	return TLazySingleton<FOo>::Get();
 * }
 * 
 * void FOo::TearDown()
 * {
 * 	TLazySingleton<FOo>::TearDown();
 * }
 */
template<class T>
class TLazySingleton final : public FLazySingleton
{
public:
	/**
	* Creates singleton once on first call.
	* Thread-safe w.r.t. other Get() calls.
	* Do not call after TearDown(). 
	*/
	static T& Get()
	{
		return GetLazy(Construct<T>).GetValue();
	}

	/** Destroys singleton. No thread must access the singleton during or after this call. */
	static void TearDown()
	{
		return GetLazy(nullptr).Reset();
	}

	/** Get or create singleton unless it's torn down */
	static T* TryGet()
	{
		return GetLazy(Construct<T>).TryGetValue();
	}

private:
	static TLazySingleton& GetLazy(void(*Constructor)(void*))
	{
		static TLazySingleton Singleton(Constructor);
		return Singleton;
	}

	/**
	 * Data: 是一个结构体，用于存储单例对象的数据，可包含单例对象的各种成员变量和状态信息
	 *		Data变量在Get()函数中被初始化，并在单例对象第一次被创建时填充。之后，Data变量可以用于访问和操作单例对象的数据
	 * Ptr: 指向单例对象的指针。用于保存单例对象的内存地址，以便在需要访问单例对象时进行引用
	 *		Ptr变量在Get()函数中被设置为指向单例对象的地址，并在单例对象被销毁时被置空
	 * 过使用Data和Ptr变量，TLazySingleton类可以在需要时延迟加载单例对象，并在单例对象的生命周期内提供对其数据的访问
	 **/
	alignas(T) unsigned char Data[sizeof(T)];
	T* Ptr;

	TLazySingleton(void(*Constructor)(void*))
	{
		if (Constructor)
		{
			Constructor(Data);
		}

		Ptr = Constructor ? (T*)Data : nullptr;
	}

#if (!defined(DISABLE_LAZY_SINGLETON_DESTRUCTION) || !DISABLE_LAZY_SINGLETON_DESTRUCTION)
	~TLazySingleton()
	{
		Reset();
	}
#endif

	T* TryGetValue()
	{
		return Ptr;
	}

	T& GetValue()
	{
		TAssert(Ptr);
		return *Ptr;
	}

	void Reset()
	{
		if (Ptr)
		{
			Destruct(Ptr);
			Ptr = nullptr;
		}
	}
};
