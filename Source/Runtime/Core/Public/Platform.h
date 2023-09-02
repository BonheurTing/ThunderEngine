#pragma once


namespace Thunder
{
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


	enum class EGfxApiType : uint32
	{
		Invalid = 0,
		D3D11,
		D3D12
	};

	
}

