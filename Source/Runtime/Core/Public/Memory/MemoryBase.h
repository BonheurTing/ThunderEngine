#pragma once

namespace Thunder
{
	class CORE_API IMalloc
	{
	public:
		virtual ~IMalloc() {}
		virtual void* Malloc(size_t count) = 0;
		virtual void* TryMalloc( size_t count) = 0;
		virtual void Free( void* original ) = 0;
	private:
	};
}
