#pragma once
#include <intrin0.inl.h>
#ifndef _TRELEASE
#define _TRELEASE 0
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
	#define THUNDER_WINDOWS 1
	#define THUNDER_POSIX 0
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__) || defined(_POSIX_VERSION)
	#define THUNDER_WINDOWS 0
	#define THUNDER_POSIX 1
#else
	#error "Unknown compiler"
#endif

namespace Thunder
{
#define FORCEINLINE __forceinline									/* Force code to be inline */

	//---------------------------------------------------------------------
	// Utility for automatically setting up the pointer-sized integer type
	//---------------------------------------------------------------------

	template<typename T32BITS, typename T64BITS, int PointerSize>
	struct SelectIntPointerType
	{
		// nothing here are is it an error if the partial specializations fail
	};

	template<typename T32BITS, typename T64BITS>
	struct SelectIntPointerType<T32BITS, T64BITS, 8>
	{
		// Select the 64 bit type.
		typedef T64BITS TIntPointer;
	};

	template<typename T32BITS, typename T64BITS>
	struct SelectIntPointerType<T32BITS, T64BITS, 4>
	{
		// Select the 32 bit type.
		typedef T32BITS TIntPointer;
	};
	
	/**
	* Generic types for almost all compilers and platforms
	**/
	struct GenericPlatformTypes
	{
	    //~ Unsigned base types
		
	    // 8-bit unsigned integer
	    typedef unsigned char 		uint8;
		
	    // 16-bit unsigned integer
	    typedef unsigned short int	uint16;
		
	    // 32-bit unsigned integer
	    typedef unsigned int		uint32;
		
	    // 64-bit unsigned integer
	    typedef unsigned long long	uint64;

	    //~ Signed base types.
		
	    // 8-bit signed integer
	    typedef	signed char			int8;
		
	    // 16-bit signed integer
	    typedef signed short int	int16;
		
	    // 32-bit signed integer
	    typedef signed int	 		int32;
		
	    // 64-bit signed integer
	    typedef signed long long	int64;

		//~ Character types
		
		// An ANSI character. 8-bit fixed-width representation of 7-bit characters.
		typedef char				ansichar;
		
		// A wide character. In-memory only. ?-bit fixed-width representation of the platform's natural wide character set. Could be different sizes on different platforms.
		typedef wchar_t				widechar;
		
		// An 8-bit character type. In-memory only. 8-bit representation. Should really be char8_t but making this the generic option is easier for compilers which don't fully support C++20 yet.
		enum utf8char : unsigned char {};
		
		// An 8-bit character type. In-memory only. 8-bit representation.
		/* UE_DEPRECATED(5.0) */[[deprecated("FPlatformTypes::CHAR8 is deprecated, please use FPlatformTypes::UTF8CHAR instead.")]]
		typedef uint8				char8;
		
		// A 16-bit character type. In-memory only.  16-bit representation. Should really be char16_t but making this the generic option is easier for compilers which don't fully support C++11 yet (i.e. MSVC).
		typedef uint16				char16;		
		
		// A 32-bit character type. In-memory only. 32-bit representation. Should really be char32_t but making this the generic option is easier for compilers which don't fully support C++11 yet (i.e. MSVC).
		typedef uint32				char32;
		
		// A switchable character. In-memory only. Either ANSICHAR or WIDECHAR, depending on a licensee's requirements.
		typedef widechar			tchar;

		// Unsigned int. The same size as a pointer.
		typedef SelectIntPointerType<uint32, uint64, sizeof(void*)>::TIntPointer UPTRINT;

		// Signed int. The same size as a pointer.
		typedef SelectIntPointerType<int32, int64, sizeof(void*)>::TIntPointer PTRINT;
	};

	//------------------------------------------------------------------
	// Transfer the platform types to global types
	//------------------------------------------------------------------

	//~ Unsigned base types.
	/// An 8-bit unsigned integer.
	typedef GenericPlatformTypes::uint8		    uint8;
	/// A 16-bit unsigned integer.
	typedef GenericPlatformTypes::uint16		uint16;
	/// A 32-bit unsigned integer.
	typedef GenericPlatformTypes::uint32		uint32;
	/// A 64-bit unsigned integer.
	typedef GenericPlatformTypes::uint64		uint64;

	//~ Signed base types.
	/// An 8-bit signed integer.
	typedef	GenericPlatformTypes::int8	    	int8;
	/// A 16-bit signed integer.
	typedef GenericPlatformTypes::int16			int16;
	/// A 32-bit signed integer.
	typedef GenericPlatformTypes::int32			int32;
	/// A 64-bit signed integer.
	typedef GenericPlatformTypes::int64			int64;

	//~ Character types.
	/// An ANSI character. Normally a signed type.
	typedef GenericPlatformTypes::ansichar		ansichar;
	/// A wide character. Normally a signed type.
	typedef GenericPlatformTypes::widechar		widechar;
	/// Either ANSICHAR or WIDECHAR, depending on whether the platform supports wide characters or the requirements of the licensee.
	typedef GenericPlatformTypes::tchar			tchar;
	// Unsigned int. The same size as a pointer.
	typedef SelectIntPointerType<uint32, uint64, sizeof(void*)>::TIntPointer UPTRINT;
	/// A signed integer the same size as a pointer
	typedef GenericPlatformTypes::PTRINT		PTRINT;
	// Unsigned int. The same size as a pointer.
	typedef UPTRINT SIZE_T;
	// Signed int. The same size as a pointer.
	typedef PTRINT SSIZE_T;

	enum class EGfxApiType : uint32
	{
		Invalid = 0,
		D3D11,
		D3D12
	};

	int TMessageBox(void* handle, const char* text, const char* caption, uint32 type);
}

