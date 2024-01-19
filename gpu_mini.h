#ifndef GPU_MINI_H
#define GPU_MINI_H

#include <stdio.h>

/*
#if defined(_WIN32)
#include <windows.h>
#else
//...
#endif
*/

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#else //< elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>


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
	//...
#endif
};

int gm_load_vkinstance(struct gm_load_vkinstance_parameters_t* parameters);
int gm_unload_vkinstance();

//int gm_load_vkdevice(struct gm_load_vkdevice_parameters_t* parameters);
//int gm_unload_vkdevice();

//int gm_load_opengl

#endif
