

#ifndef COMPILER_HLSLCC
#define COMPILER_HLSLCC 0
#endif

#ifndef COMPILER_HLSL
#define COMPILER_HLSL 0
#endif

#ifndef COMPILER_GLSL
#define COMPILER_GLSL 0
#endif

#ifndef COMPILER_GLSL_ES2
#define COMPILER_GLSL_ES2 0
#endif

#ifndef COMPILER_GLSL_ES3_1
#define COMPILER_GLSL_ES3_1 0
#endif

#ifndef COMPILER_GLSL_ES3_1_EXT
#define COMPILER_GLSL_ES3_1_EXT 0
#endif

#ifndef COMPILER_METAL
#define COMPILER_METAL 0
#endif

#ifndef COMPILER_SUPPORTS_ATTRIBUTES
#define COMPILER_SUPPORTS_ATTRIBUTES 0
#endif

#ifndef PLATFORM_SUPPORTS_SRV_UB
#define PLATFORM_SUPPORTS_SRV_UB 0
#endif

#ifndef SM5_PROFILE
#define SM5_PROFILE 0
#endif

#ifndef SM4_PROFILE
#define SM4_PROFILE 0
#endif

#ifndef OPENGL_PROFILE
#define OPENGL_PROFILE 0
#endif

#ifndef ES2_PROFILE
#define ES2_PROFILE 0
#endif

#ifndef ES3_1_PROFILE
#define ES3_1_PROFILE 0
#endif

#ifndef METAL_PROFILE
#define METAL_PROFILE 0
#endif

#ifndef METAL_MRT_PROFILE
#define METAL_MRT_PROFILE 0
#endif

#ifndef METAL_SM5_NOTESS_PROFILE
#define METAL_SM5_NOTESS_PROFILE 0
#endif

#ifndef METAL_SM5_PROFILE
#define METAL_SM5_PROFILE 0
#endif

#ifndef COMPILER_VULKAN
#define	COMPILER_VULKAN 0
#endif

#ifndef VULKAN_PROFILE
#define	VULKAN_PROFILE 0
#endif

#ifndef VULKAN_PROFILE_SM4
#define	VULKAN_PROFILE_SM4 0
#endif

#ifndef VULKAN_PROFILE_SM5
#define	VULKAN_PROFILE_SM5 0
#endif

#ifndef IOS
#define IOS 0
#endif

#ifndef MAC
#define MAC 0
#endif


// Values of FEATURE_LEVEL.
#define FEATURE_LEVEL_ES2	1
#define FEATURE_LEVEL_ES3_1	2
#define FEATURE_LEVEL_SM3	3
#define FEATURE_LEVEL_SM4	4
#define FEATURE_LEVEL_SM5	5
#define FEATURE_LEVEL_MAX	6

#define FEATURE_LEVEL FEATURE_LEVEL_SM5


#define STENCIL_COMPONENT_SWIZZLE .x