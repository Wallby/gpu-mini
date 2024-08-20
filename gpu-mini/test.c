#define GM_SAFETY
#include <gpu_mini.h>
#include <window_mini.h>
#include <test_mini.h>
#include <clock_mini.h>


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

int window;

struct wm_info_t infoAboutWindowMini;
struct wm_info_about_window_t infoAboutWindow;

//*****************************************************************************

enum
{
	ETest1Progress_LoadedVulkan = 1,
	ETest1Progress_LoadedVkinstance
};
#define ETest1Progress_All ETest1Progress_LoadedVkinstance
int test_1()
{
	int progress = 0;
	do
	{
		//if(gm_load_vulkan(&gm_load_vulkan_parameters_default) != 1)
		if(gm_load_vulkan((struct gm_load_vulkan_parameters_t*)&gm_load_vulkan_parameters_default) != 1)
		{
			break;
		}
		progress = ETest1Progress_LoadedVulkan;
		
		struct gm_load_vkinstance_parameters_t loadVkinstanceParameters;
#if defined(_WIN32)
		loadVkinstanceParameters.win32.hinstance.a = infoAboutWindowMini.win32.hinstance.a;
		loadVkinstanceParameters.win32.hwnd.a = infoAboutWindow.win32.hwnd.a;
#else //< #elif defined(__linux__)
		loadVkinstanceParameters.xlib.display.a = infoAboutWindowMini.xlib.display.a;
		loadVkinstanceParameters.xlib.window.a = infoAboutWindow.xlib.window.a;
#endif
		if(gm_load_vkinstance(&loadVkinstanceParameters) != 1)
		{
			break;
		}
		progress = ETest1Progress_LoadedVkinstance;
		
		struct gm_info_about_vulkan_t infoAboutVulkan;
		if(gm_get_info_about_vulkan(&infoAboutVulkan) != 1)
		{
			break;
		}
		
		// TODO: replace with press key to continue?
		double a = cm_get_seconds();
		double b = a;
		while(b - a < 0.5f)
		{
			wm_poll();
			if(bQuit == 1)
			{
				break;
			}

			b = cm_get_seconds();
		}
		/*
		if(bQuit == 1)
		{
			break;
		}
		*/
	} while(0);
	if(progress >= ETest1Progress_LoadedVkinstance)
	{
		gm_unload_vkinstance();
	}
	if(progress >= ETest1Progress_LoadedVulkan)
	{
		gm_unload_vulkan();
	}

	if(progress < ETest1Progress_All)
	{
		return 0;
	}
	if(bQuit == 1)
	{
		fputs("error: window was closed during test\n", stderr);
		return 0;
	}

	return 1;
}

enum
{
	ETest2Progress_LoadedDirectx11 = 1
};
#define ETest2Progress_All ETest2Progress_LoadedDirectx11
int test_2()
{
	int progress = 0;
	do
	{
		struct gm_load_directx11_parameters_t loadDirectx11Parameters = gm_load_directx11_parameters_default;
		if(gm_load_directx11(&loadDirectx11Parameters) != 1)
		{
			break;
		}
		progress = ETest2Progress_LoadedDirectx11;

		// TODO: replace with press key to continue?
		double a = cm_get_seconds();
		double b = a;
		while(b - a < 0.5f)
		{
			wm_poll();
			if(bQuit == 1)
			{
				break;
			}

			b = cm_get_seconds();
		}
		/*
		if(bQuit == 1)
		{
			break;
		}
		*/
	} while(0);
	if(progress >= ETest2Progress_LoadedDirectx11)
	{
		gm_unload_directx11();
	}

	if(progress < ETest2Progress_All)
	{
		return 0;
	}
	if(bQuit == 1)
	{
		fputs("error: window was closed during test\n", stderr);
		return 0;
	}

	return 1;
}

//*****************************************************************************

enum
{
	EProgress_LoadedWindowMini = 1,
	EProgress_AddedWindow
};
#define EProgress_All EProgress_AddedWindow
int main(int argc, char** argv)
{
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
		
		if(wm_get_info_about_window(window, &infoAboutWindow) != 1)
		{
			break;
		}

		gm_set_on_print(&on_print);
		
		TM_TEST2(1);
		TM_TEST2(2);
		
		gm_unset_on_print();
	} while(0);

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
