#pragma once


#define PLATFORM_SUPPORTS_MESH_SHADERS						1

#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
	#define PLATFORM_CPU_ARM_FAMILY							1
	#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON			1
	#define PLATFORM_ENABLE_VECTORINTRINSICS				1
#elif (defined(_M_IX86) || defined(_M_X64))
	#define PLATFORM_CPU_X86_FAMILY							1
	#define PLATFORM_ENABLE_VECTORINTRINSICS				1
#endif

// Alignment.
#if defined(__clang__)
	#define GCC_PACK(n) __attribute__((packed,aligned(n)))
	#define GCC_ALIGN(n) __attribute__((aligned(n)))
	#if defined(_MSC_VER)
		#define MS_ALIGN(n) __declspec(align(n)) // With -fms-extensions, Clang will accept either alignment attribute
	#endif
#else
	#define MS_ALIGN(n) __declspec(align(n))
#endif

// Prefetch
#define PLATFORM_CACHE_LINE_SIZE	64

// DLL export and import definitions
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)