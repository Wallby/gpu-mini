include makefile_mini.mk

$(call mm_start_parameters_t,a)
$(call mm_start,a)

$(call mm_add_library_parameters_t,b)
b.filetypes:=EMMLibraryfiletype_Static
b.c:=gpu_mini.c
b.h:=gpu_mini.h
ifeq ($(MM_OS),windows)
b.hFolders:=$(VULKAN_SDK)/Include/
else #< else ifeq ($(MM_OS),chromeos)
#...
endif
$(call mm_add_library,gpu-mini,b)

$(call mm_stop_parameters_t,c)
$(call mm_stop,c)
