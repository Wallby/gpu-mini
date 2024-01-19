#define GM_SAFETY
#include <gpu_mini.h>

#include <window_mini.h>


void on_print(char* a, FILE* b)
{
	fputs(a, b);
}

int bQuit = 0;
int on_window_closed(int window)
{
	bQuit = 1;
	
	return 1;
}

enum
{
	EProgress_LoadedWindowMini = 1,
	EProgress_AddedWindow,
	EProgress_LoadedVulkan,
	EProgress_LoadedVkinstance,
	EProgress_All
};
int main(int argc, char** argv)
{
	int window;
	
	int progress = 0;
	do
	{
		wm_set_on_print(&on_print);
		
		wm_set_on_window_closed(&on_window_closed);
		
		if(wm_load() != 1)
		{
			break;
		}
		progress = EProgress_LoadedWindowMini;
		
		struct wm_info_t infoAboutWindowMini;
		if(wm_get_info(&infoAboutWindowMini) != 1)
		{
			break;
		}
		
		struct wm_add_window_parameters_t windowParameters = wm_add_window_parameters_default;
		struct wm_window_source_t windowSource = wm_window_source_default;
		if(wm_add_window(&windowParameters, &windowSource, &window) != 1)
		{
			break;
		}
		progress = EProgress_AddedWindow;
		
		struct wm_info_about_window_t infoAboutWindow;
		if(wm_get_info_about_window(window, &infoAboutWindow) != 1)
		{
			break;
		}
		
		gm_set_on_print(&on_print);
		
		//if(gm_load_vulkan(&gm_load_vulkan_parameters_default) != 1)
		if(gm_load_vulkan((struct gm_load_vulkan_parameters_t*)&gm_load_vulkan_parameters_default) != 1)
		{
			break;
		}
		progress = EProgress_LoadedVulkan;
		
		struct gm_load_vkinstance_parameters_t loadVkinstanceParameters;
#if defined(_WIN32)
		loadVkinstanceParameters.win32.hinstance.a = infoAboutWindowMini.win32.hinstance.a;
		loadVkinstanceParameters.win32.hwnd.a = infoAboutWindow.win32.hwnd.a;
#else //< #elif defined(__linux__)
		//...
#endif
		if(gm_load_vkinstance(&loadVkinstanceParameters) != 1)
		{
			break;
		}
		progress = EProgress_LoadedVkinstance;
		
		struct gm_info_about_vulkan_t infoAboutVulkan;
		if(gm_get_info_about_vulkan(&infoAboutVulkan) != 1)
		{
			break;
		}
		
		while(bQuit != 1)
		{
			wm_poll();
		}
		
		progress = EProgress_All;
	} while(0);
	if(progress >= EProgress_LoadedVkinstance)
	{
		gm_unload_vkinstance();
	}
	if(progress >= EProgress_LoadedVulkan)
	{
		gm_unload_vulkan();
	}
	gm_unset_on_print();
	if(progress >= EProgress_AddedWindow)
	{
		wm_remove_window(window);
	}
	if(progress >= EProgress_LoadedWindowMini)
	{
		wm_unload();
	}
	wm_unset_on_print();
	if(progress != EProgress_All)
	{
		return 1;
	}
	
	return 0;
}
