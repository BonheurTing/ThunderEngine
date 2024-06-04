#pragma once
#include <atomic>
#include <xatomic.h>
#include <shared_mutex>
#include "Assertion.h"
#include "Platform.h"
#include "PlatformProcess.h"


namespace Thunder
{
	template<typename MutexType>
	class TLockGuard
	{
	public:
		TLockGuard(TLockGuard&&) = delete;
		TLockGuard(const TLockGuard&) = delete;
		TLockGuard& operator=(const TLockGuard&) = delete;
		TLockGuard& operator=(TLockGuard&&) = delete;

		_NODISCARD_CTOR TLockGuard(MutexType& InMutex)
			: Mutex(&InMutex)
		{
			TAssert(Mutex);
			Mutex->Lock();
		}

		~TLockGuard()
		{
			Unlock();
		}

		void Unlock()
		{
			if (Mutex)
			{
				Mutex->Unlock();
				Mutex = nullptr;
			}
		}

	private:
		MutexType* Mutex;
	};

	enum class ERWLockType : uint8
	{
		ReadOnly = 0,
		ReadWrite
	};

	template<typename RWLockType>
	class TRWLockGuard
	{
	public:
		_NODISCARD_CTOR explicit TRWLockGuard(RWLockType& InLockObject, ERWLockType InLockType)
			: LockObject(InLockObject)
			, LockType(InLockType)
		{
			if (LockType == ERWLockType::ReadOnly)
			{
				LockObject.ReadLock();
			}
			else
			{
				LockObject.WriteLock();
			}
		}
	
		~TRWLockGuard()
		{
			if (LockType == ERWLockType::ReadOnly)
			{
				LockObject.ReadUnlock();
			}
			else
			{
				LockObject.WriteUnlock();
			}
		}
	
	private:
		TRWLockGuard(TRWLockGuard&&) = delete;
		TRWLockGuard(const TRWLockGuard&) = delete;
		TRWLockGuard& operator=(const TRWLockGuard&) = delete;
		TRWLockGuard& operator=(TRWLockGuard&&) = delete;

	private:
		RWLockType& LockObject;
		ERWLockType LockType;
	};

	class SpinLock
	{
	public:

		SpinLock() = default;

		void Lock()
		{
			while (true)
			{
				if (!bFlag.exchange(true, std::memory_order_acquire))
				{
					break;
				}

				while (bFlag.load(std::memory_order_relaxed))
				{
					FPlatformProcess::CoreYield();
				}
			}
		}

		bool TryLock()
		{
			return !bFlag.exchange(true, std::memory_order_acquire);
		}

		void Unlock()
		{
			bFlag.store(false, std::memory_order_release);
		}

		TLockGuard<SpinLock> Guard()
		{
			return TLockGuard<SpinLock>(*this);
		}

	private:
		std::atomic<bool> bFlag{ false };
	};

	class RecursiveLock
	{
	public:
		void Lock()
		{
			Mutex.lock();
		}

		bool TryLock()
		{
			return Mutex.try_lock();
		}

		void Unlock()
		{
			Mutex.unlock();
		}

		TLockGuard<RecursiveLock> Guard()
		{
			return TLockGuard<RecursiveLock>(*this);
		}

	private:
		std::recursive_mutex Mutex;
	};

	class SharedLock
	{
		public:
			void ReadLock()
			{
				Mutex.lock_shared();
			}

			void WriteLock()
			{
				Mutex.lock();
			}

			void ReadUnlock()
			{
				Mutex.unlock_shared();
			}

			void WriteUnlock()
			{
				Mutex.unlock();
			}

			TRWLockGuard<SharedLock> Read()
			{
				return TRWLockGuard<SharedLock>(*this, ERWLockType::ReadOnly);
			}
			TRWLockGuard<SharedLock> Write()
			{
				return TRWLockGuard<SharedLock>(*this, ERWLockType::ReadWrite);
			}
		private:
			std::shared_mutex Mutex;
	};
	
}
