include makefile_mini.mk

$(call mm_start_parameters_t,a)
a.ignoredbinaries:=^test$(MM_EXECUTABLE_EXTENSION)$$
$(call mm_start,a)

$(call mm_add_library_parameters_t,b)
b.filetypes:=EMMLibraryfiletype_Static
b.c:=gpu_mini.c
b.h:=gpu_mini.h
ifndef OS #< linux
else #< windows
b.hFolders:=$(VULKAN_SDK)/Include/
endif
$(call mm_add_library,gpu-mini,b)

$(call mm_add_executable_parameters_t,c)
c.c:=test.c
#c.libraries:=gpu-mini :window-mini
c.libraries:=gpu-mini
c.hFolders:=../window-mini/
c.libFolders:=../window-mini/
c.lib:=window-mini
ifndef OS #< linux
else #< windows
c.hFolders+=$(VULKAN_SDK)/Include/
c.libFolders+=$(VULKAN_SDK)/Lib/
c.lib+=gdi32 vulkan-1
endif
$(call mm_add_executable,test,c)

$(call mm_add_test_parameters_t,d)
b.executables:=test
$(call mm_add_test,test,b)

$(call mm_stop_parameters_t,d)
$(call mm_stop,d)
