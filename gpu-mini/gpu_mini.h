#ifndef GPU_MINI_H
#define GPU_MINI_H

#include <stdio.h>

#if defined(_WIN32)
#define COBJMACROS //< because vulkan.h includes windows.h, has to be defined here
//#define WIDL_C_INLINE_WRAPPERS

#define VK_USE_PLATFORM_WIN32_KHR
#else //< elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>

#if defined(_WIN32)
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#else //< #elif defined(__linux__)
//...
#endif


//void(*on_print)(char* a, FILE* b);
void gm_set_on_print(void(*a)(char*,FILE*));
void gm_unset_on_print();

//*****************************************************************************
//                                   vulkan
//*****************************************************************************

struct gm_info_about_vulkan_t
{
	int apiVersion;
	struct
	{
		VkInstance a;
	} vkinstance;
};

enum
{
	EGMLoadVulkanParametersFlag_Safety = 1
};

struct gm_load_vulkan_parameters_t
{
	int flags;
	// NOTE: *ApiVersion == 0 is equivalent to VK_API_VERSION_1_0
	//       ^
	//       consistent with "providing an apiVersion of 0 is equivalent to..
	//       .. providing an apiVersion of VK_MAKE_API_VERSION(0,1,0,0)"
	//       ^
	//       https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
	int minApiVersion;
	// NOTE: maxApiVersion == -1 is equivalent to *ApiVersion ==..
	//       .. <gm_info_about_vulkan_t>.apiVersion (i.e. installed api..
	//       .. version)
	int maxApiVersion;
};
static const struct gm_load_vulkan_parameters_t gm_load_vulkan_parameters_default = {
#ifdef GM_SAFETY
		.flags = EGMLoadVulkanParametersFlag_Safety,
#else
		.flags = 0,
#endif
		.minApiVersion = 0,
		.maxApiVersion = -1
	};

int gm_load_vulkan(struct gm_load_vulkan_parameters_t* parameters);
int gm_unload_vulkan();

int gm_get_info_about_vulkan(struct gm_info_about_vulkan_t* infoAboutVulkan);

struct gm_load_vkinstance_parameters_t
{
#if defined(_WIN32)
	struct
	{
		struct
		{
			HINSTANCE a;
		} hinstance;
		struct
		{
			HWND a;
		} hwnd;
	} win32;
#else //< #elif defined(__linux__)
	struct
	{
		struct
		{
			Display* a;
		} display;
		struct
		{
			Window a;
		} window;
	} xlib;
#endif
};

int gm_load_vkinstance(struct gm_load_vkinstance_parameters_t* parameters);
int gm_unload_vkinstance();

//int gm_load_vkdevice(struct gm_load_vkdevice_parameters_t* parameters);
//int gm_unload_vkdevice();

//*****************************************************************************
//                                   opengl
//*****************************************************************************

//int gm_load_opengl

#ifdef _WIN32
//*****************************************************************************
//                                 directx 12
//*****************************************************************************

//int gm_load_directx12

//*****************************************************************************
//                                 directx 11
//*****************************************************************************

struct gm_info_about_directx11_t
{
	D3D_FEATURE_LEVEL featureLevel;
	struct
	{
		ID3D11Device* a;
	} id3d11device;
	struct
	{
		ID3D11DeviceContext* a;
	} id3d11devicecontext;
};

enum
{
	EGMLoadDirectx11ParametersFlag_Safety = 1
};

struct gm_load_directx11_parameters_t
{
	int flags;
	DXGI_GPU_PREFERENCE gpuPreference;
	D3D_FEATURE_LEVEL minFeatureLevel;
	D3D_FEATURE_LEVEL maxFeatureLevel;
};
static const struct gm_load_directx11_parameters_t gm_load_directx11_parameters_default = {
#ifdef GM_SAFETY
		.flags = EGMLoadDirectx11ParametersFlag_Safety,
#else
		.flags = 0,
#endif
		.gpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED,
		.minFeatureLevel = D3D_FEATURE_LEVEL_9_1,
		.maxFeatureLevel = D3D_FEATURE_LEVEL_11_1
	};

int gm_load_directx11(struct gm_load_directx11_parameters_t* parameters);
int gm_unload_directx11();

int gm_get_info_about_directx11(struct gm_info_about_directx11_t* infoAboutDirectx11);
#endif

#endif
