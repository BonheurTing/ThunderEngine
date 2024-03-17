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