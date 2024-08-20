#include "gpu_mini.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


static void get_hresult_to_string(HRESULT a, int* b, char* c)
{
	char* d;
	// NOTE: FORMAT_MESSAGE_MAX_WIDTH_MASK such that "[FormatMessageA]..
	//       .. ignores regular line breaks in the message definition..
	//       .. text"
	//       ^
	//       https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessagea
	// NOTE: currently assuming US English is always available (I have no..
	//       .. proof whether or not this is so)
	//       v
	if(FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, a, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&d, 0, NULL) != 0)
	{
		if(c == NULL)
		{
			*b = strlen(d) + 1;
		}
		else
		{
			strncpy(c, d, *b);
		}
		LocalFree(d);
	}
	else
	{
		if(c == NULL)
		{
			// https://stackoverflow.com/questions/29087129/how-to-calculate-the-length-of-output-that-sprintf-will-generate
			*b = snprintf(NULL, 0, "%u", a) + 1;
		}
		else
		{
			snprintf(c, *b, "%u", a);
		}
	}
}

struct similarity_t
{
	int index1;
	int index2;
};

// NOTE: returns 1 if not different
//       returns 0 if different
#define predicate_t int(*)(void* a, void* b)

// NOTE: returns 1 if memcmp(a, b, elementSize) == 0
//       returns 0 otherwise
// NOTE: for consistency with am_predicate_not_memcmp
#define predicate_memcmp NULL

// NOTE: returns 1 if memcmp(a, b, elementSize) != 0
//       returns 0 otherwise
static int predicate_not_memcmp(void* a, void* b)
{
	fputs("error: predicate_not_memcmp should not be called (see comment in array_mini.c)\n", stderr);
	
	return 0;
}

static int predicate2(int elementSize, int(*predicate)(void*, void*), void* element, void* query)
{
	if(predicate == NULL)
	{
		return memcmp(element, query, elementSize) == 0 ? 1 : 0;
	}
	else if(predicate == predicate_not_memcmp)
	{
		return memcmp(element, query, elementSize) != 0 ? 1 : 0;
	}
	else
	{
		return predicate(element, query);
	}
}

// NOTE: assumes..
//       .. numElements > 0
//       .. index != NULL
// NOTE: int(*predicate)(void* element, void* query)
static int search_first_in(int elementSize, int(*predicate)(void*, void*), int numElements, void* elements, void* query, int* index)
{
	for(int i = 0; i < numElements; ++i)
	{
		void* element = elements + elementSize * i;
	
		int bAreNotDifferent;
		if(predicate == NULL)
		{
			bAreNotDifferent = memcmp(element, query, elementSize) == 0 ? 1 : 0;
		}
		else if (predicate == predicate_not_memcmp)
		{
			bAreNotDifferent = memcmp(element, query, elementSize) != 0 ? 1 : 0;
		}
		else
		{
			bAreNotDifferent = predicate(element, query);
		}
		if(bAreNotDifferent == 1)
		{
			*index = i;
			return 1;
		}
	}
	
	return 0;
}
//#define search_first_in2(predicate, numElements, elements, query, index) search_first_in(sizeof *elements, (predicate_t)predicate, numElements, (void*)elements, (void*)query, index)

// NOTE: assumes..
//       .. numElements > 0
// NOTE: int(*predicate)(void* element, void* query)
static int is_in(int elementSize, int(*predicate)(void*, void*), int numElements, void* elements, void* query)
{
	int a;
	return search_first_in(elementSize, predicate, numElements, elements, query, &a);
}
#define is_in2(predicate, numElements, elements, query) is_in(sizeof *elements, (predicate_t)predicate, numElements, (void*)elements, (void*)query)

// NOTE: assumes..
//       .. numElementsToAppend > 0
static void add_or_append_elements(int elementSize, int* numElements, void** elements, int numElementsToAppendOrAdd, void* elementsToAppendOrAdd)
{
	void* a = *elements;
	//*elements = (void*)new char[elementSize * ((*numElements) + numElementsToAppendOrAdd)];
	*elements = malloc(elementSize * ((*numElements) + numElementsToAppendOrAdd));
	if(*numElements > 0)
	{
		memcpy(*elements, a, elementSize * (*numElements));
		//delete a;
		free(a);
	}
	memcpy(*elements + (elementSize * (*numElements)), elementsToAppendOrAdd, elementSize * numElementsToAppendOrAdd);
	*numElements += numElementsToAppendOrAdd;
}
#define add_or_append_element(elementSize, numElements, elements, elementToAppendOrAdd) add_or_append_elements(elementSize, numElements, elements, 1, elementToAppendOrAdd)
#define add_or_append_element2(numElements, elements, elementToAppendOrAdd) add_or_append_element(sizeof **elements, numElements, (void**)elements, (void*)elementToAppendOrAdd)

// NOTE: remove element(s) where there is/are element(s)
// NOTE: assumes..
//       .. numElements > 0
void remove_elements(int elementSize, int* numElements, void** elements)
{
	//delete *elements;
	free(*elements);
	*numElements = 0;
}
#define remove_elements2(numElements, elements) remove_elements(sizeof **elements, numElements, (void**)elements)

// NOTE: uniques within one array
// NOTE: assumes..
//       .. numElements > 0
//       .. numUniques != NULL
//       .. lengthof uniques == numElements
// NOTE: int(*predicate)(void* element, void* query)
void get_uniques(int elementSize, int(*predicate)(void*, void*), int numElements, void* elements, int* numUniques, int** indexPerUnique)
{
	*numUniques = 0;
	for(int i = 0; i < numElements; ++i)
	{
		void* element1 = elements + elementSize * i;
		
		int bIsDuplicate = 0;
		for(int j = 0; j < numElements; ++j)
		{
			if(i == j)
			{
				continue;
			}
			
			void* element2 = elements + elementSize * j;
			
			if(predicate2(elementSize, predicate, element1, element2) == 1)
			{
				bIsDuplicate = 1;
				break;
			}
		}
		if(bIsDuplicate == 1)
		{
			continue;
		}
		
		(*indexPerUnique)[*numUniques] = i;
		++(*numUniques);
	}
}
#define get_uniques2(predicate, numElements, elements, numUniques, indexPerUnique) get_uniques(sizeof *elements, (predicate_t)predicate, numElements, (void*)elements, numUniques, indexPerUnique)

/*
// NOTE: assumes..
//       .. numElements > 0
// NOTE: int(*predicate)(void* element, void* query)
void move_to_back(int elementSize, int(*predicate)(void*, void*), int numElements, void** elements, void* query, int* numElementsMovedToBack)
{
	*numElementsMovedToBack = 0;
	for(int i = 0; i < numElements - *numElementsMovedToBack;)
	{
		void* element1 = *elements + elementSize * i;
	
		int bMoveToBack1 = predicate2(elementSize, predicate, element1, query);
		if(bMoveToBack1 == 1)
		{
			int indexToNextElementToNotMoveToBack = -1;
			for(int j = i + 1; j < numElements - *numElementsMovedToBack; ++j)
			{
				void* element2 = *elements + elementSize * j;
			
				int bMoveToBack2 = predicate2(elementSize, predicate, element2, query);
				if(bMoveToBack2 == 0)
				{
					indexToNextElementToNotMoveToBack = j;
					break;
				}
			}
			if(indexToNextElementToNotMoveToBack != -1)
			{
				int numElementsToMoveToBack = indexToNextElementToNotMoveToBack - i;
				
				char elementsToMoveToBack[elementSize * numElementsToMoveToBack]; //< 0 length array is not officially allowed in C, but guaranteed that numElementsToMoveToBack is >= 1 here
				/// backup front elements
				memcpy(elementsToMoveToBack, *elements + elementSize * i, elementSize * numElementsToMoveToBack);
				
				int lastNumElementsToMoveToFront = numElements - indexToNextElementToNotMoveToBack;

				// move back elements to front
				memcpy(*elements + elementSize * i, *elements + elementSize * indexToNextElementToNotMoveToBack, elementSize * lastNumElementsToMoveToFront);
				// move front elements from backup to back
				memcpy(*elements + elementSize * (i + lastNumElementsToMoveToFront), elementsToMoveToBack, elementSize * numElementsToMoveToBack);
				
				*numElementsMovedToBack += numElementsToMoveToBack;
			}
			else
			{
				int numElementsToMoveBack = (numElements - *numElementsMovedToBack) - i;
				// ^
				// because if i == 0.. numElementsToMoveToBack should ==..
				// .. numElements - *numElementsMovedToBack
				
				// ~don't actually move these elements as already at back~
				// ^
				// do move these back (but not to back as already at back)..
				// .. to assure order is preserved
				
				int lastNumElementsToMoveFront = *numElementsMovedToBack;
				
				if(lastNumElementsToMoveFront > 0)
				{
					char elementsToMoveBack[elementSize * numElementsToMoveBack]; //< 0 length array is not officially allowed in C, but guaranteed that numElementsToMoveToBack is >= 1 here
					memcpy(elementsToMoveBack, *elements + elementSize * i, elementSize * numElementsToMoveBack);
					
					memcpy(*elements + elementSize * i, *elements + elementSize * (i + numElementsToMoveBack), elementSize * lastNumElementsToMoveFront);
					memcpy(*elements + elementSize * (i + lastNumElementsToMoveFront), elementsToMoveBack, elementSize * numElementsToMoveBack);
				}
				
				*numElementsMovedToBack += numElementsToMoveBack;
			}
		}
		else
		{
			++i;
		}
	}
}
#define move_to_back2(predicate, numElements, elements, query, numElementsMovedToBack) move_to_back(sizeof **elements, (predicate_t)predicate, numElements, (void**)elements, (void*)query, numElementsMovedToBack)
*/

//*****************************************************************************

int predicate$char$(char* a, char* b)
{
	return strcmp(a, b) == 0 ? 1 : 0;
}

//*****************************************************************************

static void(*on_print)(char* a, FILE* b) = NULL;
#define on_print2(a, b) if(on_print != NULL) { on_print(a, b); }
void on_printf(FILE* a, char* b, ...)
{
	va_list c;
	va_start(c, b);
	
	int d = vsnprintf(NULL, 0, b, c);
	
	char e[d + 1];
	
	vsprintf(e, b, c);
	
	va_end(c);
	
	on_print(e, a);
}
#define on_printf2(a, b, ...) if(on_print != NULL) { on_printf(a, b __VA_OPT__(,) __VA_ARGS__); }

void gm_set_on_print(void(*a)(char*, FILE*))
{
	on_print = a;
}
void gm_unset_on_print()
{
	on_print = NULL;
}

//*****************************************************************************
//                                   vulkan
//*****************************************************************************

typedef struct
{
	VkStructureType sType;
	const void* pNext;
} VkStructure;

static const VkDebugUtilsMessengerCreateInfoEXT VK_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = 0,
		.pUserData = NULL
	};

static const VkApplicationInfo VK_APPLICATION_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = NULL,
		.applicationVersion = 0,
		.pEngineName = NULL,
		.engineVersion = 0
	};

static const VkInstanceCreateInfo VK_INSTANCE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = NULL,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = NULL
	};

static const VkPhysicalDeviceFeatures VK_PHYSICAL_DEVICE_FEATURES_DEFAULT = {};

static const VkDeviceQueueCreateInfo VK_DEVICE_QUEUE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkDeviceCreateInfo VK_DEVICE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = NULL,
		.pEnabledFeatures = NULL
	};

#if defined(_WIN32)
static const VkWin32SurfaceCreateInfoKHR VK_WIN32_SURFACE_CREATE_INFO_KHR_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0
	};
#else //< #elif defined(__linux__)
static const VkXlibSurfaceCreateInfoKHR VK_XLIB_SURFACE_CREATE_INFO_KHR_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0
	};
#endif

static const VkSwapchainCreateInfoKHR VK_SWAPCHAIN_CREATE_INFO_KHR_WINDOW_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		//.queueFamilyIndexCount = 0,
		//.pQueueFamilyIndices= NULL,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

static const VkSwapchainCreateInfoKHR VK_SWAPCHAIN_CREATE_INFO_KHR_VR_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.imageArrayLayers = 2,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		//.queueFamilyIndexCount = 0,
		//.pQueueFamilyIndices = NULL,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_FALSE,
		.oldSwapchain = VK_NULL_HANDLE
	};

static const VkComponentMapping VK_COMPONENT_MAPPING_DEFAULT = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};

static const VkImageViewCreateInfo VK_IMAGE_VIEW_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.components = VK_COMPONENT_MAPPING_DEFAULT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount= VK_REMAINING_ARRAY_LAYERS
	};

static const VkShaderModuleCreateInfo VK_SHADER_MODULE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkPipelineShaderStageCreateInfo VK_PIPELINE_SHADER_STAGE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pName = "main",
		.pSpecializationInfo = NULL
	};

static const VkPipelineVertexInputStateCreateInfo VK_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL
	};

static const VkPipelineInputAssemblyStateCreateInfo VK_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.primitiveRestartEnable = VK_FALSE
	};

static const VkViewport VK_VIEWPORT_DEFAULT = {
		.x = 0.0f,
		.y = 0.0f,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

static const VkPipelineDynamicStateCreateInfo VK_PIPELINE_DYNAMIC_STATE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = 0,
		.pDynamicStates = NULL
	};

static const VkOffset2D VK_OFFSET2D_NONE = {
		.x = 0,
		.y = 0
	};

static const VkRect2D VK_RECT2D_DEFAULT = {
		.offset = VK_OFFSET2D_NONE
	};

static const VkPipelineViewportStateCreateInfo VK_PIPELINE_VIEWPORT_STATE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkPipelineRasterizationStateCreateInfo VK_PIPELINE_RASTERIZATION_STATE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

static const VkPipelineMultisampleStateCreateInfo VK_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO_DISABLED = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

static const VkPipelineDepthStencilStateCreateInfo VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO_DISABLED = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.depthTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

static const VkColorComponentFlagBits VK_COLOR_COMPONENT_FLAG_BITS_ALL = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

static const VkPipelineColorBlendAttachmentState VK_PIPELINE_COLOR_BLEND_ATTACHMENT_STATE_OPAQUE = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_FLAG_BITS_ALL
	};

static const VkPipelineColorBlendStateCreateInfo VK_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO_OPAQUE = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &VK_PIPELINE_COLOR_BLEND_ATTACHMENT_STATE_OPAQUE,
	};

static const VkPipelineLayoutCreateInfo VK_PIPELINE_LAYOUT_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

static const VkAttachmentDescription VK_ATTACHMENT_DESCRIPTION_DEFAULT = {
		.flags = 0,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

static const VkSubpassDescription VK_SUBPASS_DESCRIPTION_DEFAULT = {
		.flags = 0,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 0,
		.pColorAttachments = NULL,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

static const VkRenderPassCreateInfo VK_RENDER_PASS_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 0,
		.pAttachments = NULL,
		.dependencyCount = 0,
		.pDependencies = NULL
	};

static const VkGraphicsPipelineCreateInfo VK_GRAPHICS_PIPELINE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pVertexInputState = NULL,
		.pInputAssemblyState = NULL,
		.pTessellationState = NULL,
		.pViewportState = NULL,
		.pRasterizationState = NULL,
		.pMultisampleState = NULL,
		.pDepthStencilState = NULL,
		.pColorBlendState = NULL,
		.pDynamicState = NULL,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

static const VkFramebufferCreateInfo VK_FRAMEBUFFER_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.layers = 1
	};

static const VkCommandPoolCreateInfo VK_COMMAND_POOL_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkCommandBufferAllocateInfo VK_COMMAND_BUFFER_ALLOCATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	};

static const VkCommandBufferBeginInfo VK_COMMAND_BUFFER_BEGIN_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL
	};

static const VkClearValue VK_CLEAR_VALUE_DEFAULT = {
		.color.float32 = { 0.0f, 0.0f, 0.0f, 1.0f },
		.depthStencil.depth = 1.0f,
		.depthStencil.stencil = 1
	};

static const VkRenderPassBeginInfo VK_RENDER_PASS_BEGIN_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderArea.offset = VK_OFFSET2D_NONE,
		.clearValueCount = 0,
		.pClearValues = NULL
	};

static const VkSemaphoreCreateInfo VK_SEMAPHORE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkFenceCreateInfo VK_FENCE_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkSubmitInfo VK_SUBMIT_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 0,
		.pCommandBuffers = NULL,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};

static const VkPresentInfoKHR VK_PRESENT_INFO_KHR_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pResults = NULL
	};

static const VkBufferCreateInfo VK_BUFFER_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE//,
		//.queueFamilyIndexCount = 0,
		//.pQueueFamilyIndices = NULL
	};

static const VkMemoryAllocateInfo VK_MEMORY_ALLOCATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL
	};

static const VkDescriptorSetLayoutBinding VK_DESCRIPTOR_SET_LAYOUT_BINDING_DEFAULT = {
		.pImmutableSamplers = NULL
	};

static const VkDescriptorSetLayoutCreateInfo VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 0,
		.pBindings = NULL
	};

static const VkDescriptorPoolCreateInfo VK_DESCRIPTOR_POOL_CREATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

static const VkDescriptorSetAllocateInfo VK_DESCRIPTOR_SET_ALLOCATE_INFO_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL
	};

static const VkDescriptorBufferInfo VK_DESCRIPTOR_BUFFER_INFO_DEFAULT = {
		.buffer = VK_NULL_HANDLE,
		.offset = 0,
		.range = VK_WHOLE_SIZE
	};

static const VkWriteDescriptorSet VK_WRITE_DESCRIPTOR_SET_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = NULL,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.pImageInfo = NULL,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL
	};

static const VkMappedMemoryRange VK_MAPPED_MEMORY_RANGE_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.pNext = NULL,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

static const VkBufferCopy VK_BUFFER_COPY_DEFAULT = {
		.srcOffset = 0,
		.dstOffset = 0
	};

static const VkBufferMemoryBarrier VK_BUFFER_MEMORY_BARRIER_DEFAULT = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = NULL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

//*****************************************************************************

enum
{
	EVulkanInstanceLayers_Validation = 0 //< "VK_LAYER_KHRONOS_validation"
};
#define NUM_VULKAN_INSTANCE_LAYERS (EVulkanInstanceLayers_Validation + 1)
static char* layerNamePerVulkanInstanceLayer[NUM_VULKAN_INSTANCE_LAYERS] = {
		"VK_LAYER_KHRONOS_validation"
	};

enum
{
	EVulkanInstanceExtension_Vkkhrdebugutils = 0 //< "VK_EXT_debug_utils" (VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
};
#define NUM_VULKAN_INSTANCE_EXTENSIONS (EVulkanInstanceExtension_Vkkhrdebugutils + 1)
static struct
{
	int layer; //< may == -1 for layerName == NULL
	char* extensionName;
} perVulkanInstanceExtension[NUM_VULKAN_INSTANCE_EXTENSIONS] = {
		{ EVulkanInstanceLayers_Validation, VK_EXT_DEBUG_UTILS_EXTENSION_NAME }
	};
// NOTE: ^
//       VkInstanceCreateInfo contains layers and extensions, not "layer per..
//       .. extension" thus guaranteed that extension name is unique

static int bIsVulkanLoaded = 0;

struct vulkan_instance_t
{
	struct
	{
		VkInstance a;
	} vkinstance;
	struct
	{
		VkDebugUtilsMessengerEXT a;
	} vkdebugutilsmessengerext;
};
static const struct vulkan_instance_t vulkan_instance_default = {
		.vkinstance.a = VK_NULL_HANDLE,
		.vkdebugutilsmessengerext.a = VK_NULL_HANDLE
	};
struct vulkan_t
{
	int flags;
	int apiVersion;
	struct vulkan_instance_t instance;
	int bIsEnabledPerInstanceLayer[NUM_VULKAN_INSTANCE_LAYERS];
	int bIsEnabledPerInstanceExtension[NUM_VULKAN_INSTANCE_EXTENSIONS];
};
static const struct vulkan_t vulkan_default = {
		.instance = vulkan_instance_default
	};

static struct vulkan_t vulkan = vulkan_default;

//*****************************************************************************

static uint32_t get_vulkan_api_version()
{
	PFN_vkEnumerateInstanceVersion a = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
	if(a == NULL)
	{
		// NOTE: "Vulkan 1.0 implementations were required to return..
		//       .. VK_ERROR_INCOMPATIBLE_DRIVER if apiVersion was larger..
		//       .. than 1.0",..
		//       .. https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
		//       "Because Vulkan 1.0 implementations may fail with..
		//       .. VK_ERROR_INCOMPATIBLE_DRIVER, applications should..
		//       .. determine the version of Vulkan available before calling..
		//       .. vkCreateInstance. If the vkGetInstanceProcAddr returns..
		//       .. NULL for vkEnumerateInstanceVersion, it is a Vulkan 1.0..
		//       .. implementation",..
		//       .. https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
		return VK_API_VERSION_1_0;
	}
	
	uint32_t b;
	if(vkEnumerateInstanceVersion(&b) != VK_SUCCESS)
	{
		on_printf2(stderr, "warning: vkEnumerateInstanceVersion != VK_SUCCESS in %s\n", __FUNCTION__);
		return VK_API_VERSION_1_0;
	}
	
	return b;
}


// NOTE: returns 1 if succeeded
//       returns 0 if failed
// NOTE: if a == stderr.. print error if failed
//       if a == stdout.. print warning if failed
//       requiredLayers[..] may == -1 to indicate a gap
//       (*optionalLayers)[..] may == -1 to indicate a gap
//       if succeeded.. will write to *numOptionalLayersAvailable
// NOTE: no such as "instance layer" as "Since version 1.0.13 of the Vulkan..
//       .. API specification [...] the Vulkan SDK device layers have been..
//       .. deprecated"
//       ^
//       https://gpuopen.com/learn/using-the-vulkan-validation-layers/
static int test_instance_layers(FILE* a, int numRequiredLayersTheresRoomFor, int* requiredLayers, int numOptionalLayersTheresRoomFor, int** optionalLayers, int* numOptionalLayersAvailable)
{
	char* warningOrError = a == stderr ? "error" : "warning";
	
	uint32_t b;
	if(vkEnumerateInstanceLayerProperties(&b, NULL) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceLayerProperties != VK_SUCCESS in %s\n", warningOrError, __FUNCTION__);
		return 0;
	}
	
	VkLayerProperties c[b];
	
	if(vkEnumerateInstanceLayerProperties(&b, c) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceLayerProperties != VK_SUCCESS in %s\n", warningOrError, __FUNCTION__);
		return 0;
	}
	
	int bIsAnyRequiredLayerNotAvailable = 0;
	for(int i = 0; i < numRequiredLayersTheresRoomFor; ++i)
	{
		if(requiredLayers[i] == -1)
		{
			continue; //< ignore gap
		}
	
		int layer = requiredLayers[i];
		char* layerName = layerNamePerVulkanInstanceLayer[layer];

		int bIsLayerAvailable = 0;
		for(int j = 0; j < b; ++j)
		{
			if(strcmp(layerName, c[j].layerName) == 0)
			{
				bIsLayerAvailable = 1;
				break;
			}
		}
		if(bIsLayerAvailable == 0)
		{
			if(a == stderr)
			{
				on_printf2(stderr, "error: required instance layer %s missing in %s\n", layerName, __FUNCTION__);
			}
			return 0;
		}
	}
	
	if(numOptionalLayersTheresRoomFor > 0)
	{
		int d = 0;
		for(int i = 0; i < numOptionalLayersTheresRoomFor; ++i)
		{
			if((*optionalLayers)[i] == -1)
			{
				continue; //< ignore gap
			}

			int layer = (*optionalLayers)[i];
			char* layerName = layerNamePerVulkanInstanceLayer[layer];

			int bIsLayerAvailable = 0;
			for(int j = 0; j < b; ++j)
			{
				if(strcmp(layerName, c[j].layerName) == 0)
				{
					bIsLayerAvailable = 1;
					++d;
					
					break;
				}
			}

			if(bIsLayerAvailable == 0)
			{
				(*optionalLayers)[i] = -1;
			}
		}
		*numOptionalLayersAvailable = d;
	}

	return 1;
}

// NOTE: returns 1 if succeeded
//       returns 0 if failed
// NOTE: if a == stderr.. print error if failed
//       if a == stdout.. print warning if failed
//       layerName may == NULL
// NOTE: assumes that layerName if any is tested
static int test_instance_extension(FILE* a, char* layerName, char* extensionName)
{
	char* warningOrError = a == stderr ? "error" : "warning";

	uint32_t b;
	// "[If] pLayerName parameter is NULL, only extensions provided by..
	// .. the Vulkan implementation or by implicitly enabled layers are..
	// .. returned",
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
	// v
	if(vkEnumerateInstanceExtensionProperties(layerName, &b, NULL) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceExtensionProperties != VK_SUCCESS in %s\n", warningOrError, __FUNCTION__); 
		return 0;
	}
	
	VkExtensionProperties c[b];
	
	if(vkEnumerateInstanceExtensionProperties(layerName, &b, c) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceExtensionProperties != VK_SUCCESS in %s\n", warningOrError, __FUNCTION__);
		return 0;
	}
	
	for(int i = 0; i < b; ++i)
	{
		if(strcmp(extensionName, c[i].extensionName) == 0)
		{
			return 1;
		}
	}
	
	return 0;
}

struct test_instance_extensions_per_extension_t
{
	int layer;
	int extension;
};
// NOTE: returns 1 if succeeded
//       returns 0 if failed
// NOTE: if a == stderr.. print error
//       if a == stdout.. print warning
//       requiredExtensions[..] may == -1 to indicate a gap
//       (*optionalExtensions)[..] may == -1 to indicate a gap
//       if succeeded..
//       .. if an optional extension at index .. is not available will set..
//          .. optionalExtensions[..] == -1
//       .. will write to *numOptionalExtensionsAvailable
// NOTE: assumes that each layer if any is tested even for optional layer(s)..
//       .. if any
static int test_instance_extensions(FILE* a, int bIsAvailablePerInstanceLayer[NUM_VULKAN_INSTANCE_EXTENSIONS], int numRequiredExtensionsTheresRoomFor, int* requiredExtensions, int numOptionalExtensionsTheresRoomFor, int** optionalExtensions, int* numOptionalExtensionsAvailable)
{
	for(int i = 0; i < numRequiredExtensionsTheresRoomFor; ++i)
	{
		if(requiredExtensions[i] == -1)
		{
			continue; //< ignore gap
		}

		int extension = requiredExtensions[i];
		char* extensionName = perVulkanInstanceExtension[extension].extensionName;

		int layer = perVulkanInstanceExtension[extension].layer;
		char* layerName = NULL;
		if(layer != -1)
		{
			layerName = layerNamePerVulkanInstanceLayer[layer];
		}

		if(test_instance_extension(a, layerName, extensionName) == 0)
		{
			// only print warning about external library call, thus in..
			// .. test_instance_extension..
			// .. vkEnumerateInstanceExtensionProperties warning is fine,..
			// .. but don't print warning about test_instance_extension fail
			// v
			if(a == stderr)
			{
				on_printf2(stderr, "error: required instance extension %s is missing (layerName == %s) in %s\n", extensionName, layerName == NULL ? "NULL" : layerName, __FUNCTION__);
			}
			return 0;
		}
	}

	if(numOptionalExtensionsTheresRoomFor != 0)
	{
		int b = 0;
		for(int i = 0; i < numOptionalExtensionsTheresRoomFor; ++i)
		{
			if((*optionalExtensions)[i] == -1)
			{
				continue; //< ignore gap
			}

			int extension = (*optionalExtensions)[i];
			char* extensionName = perVulkanInstanceExtension[extension].extensionName;

			int layer = perVulkanInstanceExtension[extension].layer;
			char* layerName = NULL;
			if(layer != -1)
			{
				if(bIsAvailablePerInstanceLayer[layer] == 0)
				{
					// disable extension because requires layer that is not..
					// .. available
					(*optionalExtensions)[i] = -1;
					continue;
				}

				layerName = layerNamePerVulkanInstanceLayer[layer];
			}

			//            optional instance extension should never print error
			//                         v
			if(test_instance_extension(stdout, layerName, extensionName) == 0)
			{
				(*optionalExtensions)[i] = -1;
				continue;
			}
			
			++b;
		}
		*numOptionalExtensionsAvailable = b;
	}

	return 1;
}

//*****************************************************************************

static VkBool32 VKAPI_PTR myVkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	// NOTE: not sure if messageSeverity can have more than one bit set
	//       v
	switch(messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		on_printf(stdout, "warning: %s\n", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		on_printf(stderr, "error: %s\n", pCallbackData->pMessage);
		break;
	default:
		fputs(pCallbackData->pMessage, stdout);
	}

	return VK_FALSE;
}

int gm_load_vulkan(struct gm_load_vulkan_parameters_t* parameters)
{
	if(bIsVulkanLoaded == 1)
	{
		return -1;
	}

	uint32_t apiVersion = get_vulkan_api_version();
	uint32_t minApiVersion;
	switch(parameters->minApiVersion)
	{
	case -1:
		minApiVersion = apiVersion;
		break;
	case 0:
		minApiVersion = VK_API_VERSION_1_0;
		break;
	}
	uint32_t maxApiVersion;
	switch(parameters->maxApiVersion)
	{
	case -1:
		maxApiVersion = apiVersion;
		break;
	case 0:
		maxApiVersion = VK_API_VERSION_1_0;
		break;
	}

	if(apiVersion < minApiVersion)
	{
		return 0;
	}
	if(apiVersion > maxApiVersion)
	{
		return 0;
	}
	
	vulkan.flags = parameters->flags;
	vulkan.apiVersion = apiVersion;
	
	bIsVulkanLoaded = 1;
	
	return 1;
}
int gm_unload_vulkan()
{
	if(bIsVulkanLoaded == 0)
	{
		return -1;
	}
	
	// IDEA: maybe allow gm_unload_vulkan to unload all vulkan variable(s)..
	//       .. if any?
	if(vulkan.instance.vkinstance.a != VK_NULL_HANDLE)
	{
		on_printf(stdout, "warning: vkinstance is loaded in %s\n", __FUNCTION__);
		return -1;
	}
	
	bIsVulkanLoaded = 0;

	return 1;
}

static void unload_vkinstance(struct vulkan_instance_t* instance);
int gm_load_vkinstance(struct gm_load_vkinstance_parameters_t* parameters)
{
	if(bIsVulkanLoaded == 0)
	{
		return -1;
	}
	
	if(vulkan.instance.vkinstance.a != VK_NULL_HANDLE)
	{
		return -1;
	}
	
	struct vulkan_instance_t instance = vulkan_instance_default;
	
	int bIsAvailablePerInstanceLayer[NUM_VULKAN_INSTANCE_LAYERS];
	int bIsAvailablePerInstanceExtension[NUM_VULKAN_INSTANCE_EXTENSIONS];

	// any layers/extensions can be added here to these below variables
	// adding an extension that requires a layer will automatically..
	// .. require the layer (thus not required to manually add the layer..
	// .. as well)
	int numRequiredLayers = 0;
	int* requiredLayers;
	int numOptionalLayers = 0;
	int* optionalLayers;
	int numRequiredExtensions = 0;
	int* requiredExtensions;
	int numOptionalExtensions = 0;
	int* optionalExtensions;
	if((vulkan.flags & EGMLoadVulkanParametersFlag_Safety) != 0)
	{
		//add_or_append_element2(&numOptionalExtensions, &optionalExtensions, &EVulkanInstanceExtension_Vkkhrdebugutils);
		int a = EVulkanInstanceExtension_Vkkhrdebugutils;
		add_or_append_element2(&numOptionalExtensions, &optionalExtensions, &a);
	}

	int bSuccess = 1;
	do
	{
		// for every extension where layer != -1.. add layer  
		for(int i = 0; i < numRequiredExtensions; ++i)
		{
			int extension = requiredExtensions[i];
		
			int layer = perVulkanInstanceExtension[extension].layer;
			if(layer == -1)
			{
				continue;
			}
			
			add_or_append_element2(&numRequiredLayers, &requiredLayers, &layer);
		}
		for(int i = 0; i < numOptionalExtensions; ++i)
		{
			int extension = optionalExtensions[i];
		
			int layer = perVulkanInstanceExtension[extension].layer;
			if(layer == -1)
			{
				continue;
			}
			
			add_or_append_element2(&numOptionalLayers, &optionalLayers, &layer);
		}
		
		// remove duplicate layer(s) if any..
		int numUniqueRequiredLayers;
		int indexPerUniqueRequiredLayer[numRequiredLayers];
		//get_uniques2(NULL, numRequiredLayers, requiredLayers, &numUniqueRequiredLayers, &indexPerUniqueRequiredLayer);
		int* a = indexPerUniqueRequiredLayer;
		get_uniques2(NULL, numRequiredLayers, requiredLayers, &numUniqueRequiredLayers, &a);
		int uniqueRequiredLayers[numUniqueRequiredLayers];
		for(int i = 0; i < numUniqueRequiredLayers; ++i)
		{
			uniqueRequiredLayers[i] = requiredLayers[indexPerUniqueRequiredLayer[i]];
		}

		// only numUniqueRequiredLayers and uniqueRequiredLayers from here

		// for optional layers..
		// .. numAvailableOptionalLayers will first be set to # unique optional layers, then modified by test_instance_layers
		// .. indexPerUniqueOptionalLayer is only used modified by get_uniques, hence unique
		// .. availableOptionalLayers will first be set using indexPerUniqueOptionalLayers, then modified by test_instance_layers
		int numAvailableOptionalLayers;
		int indexPerUniqueOptionalLayer[numOptionalLayers];
		//get_uniques2(NULL, numOptionalLayers, optionalLayers, &numAvailableOptionalLayers, &indexPerUniqueOptionalLayer);
		int* b = indexPerUniqueOptionalLayer;
		get_uniques2(NULL, numOptionalLayers, optionalLayers, &numAvailableOptionalLayers, &b);
		int availableOptionalLayers[numAvailableOptionalLayers];
		for(int i = 0; i < numAvailableOptionalLayers; ++i)
		{
			availableOptionalLayers[i] = optionalLayers[indexPerUniqueOptionalLayer[i]];
		}
		
		// only numAvailableOptionalLayers and availableOptionalLayers from here

		// test instance layers
		int numUniqueOptionalLayersTheresRoomFor = numAvailableOptionalLayers; //< exception to above "only ... from here" as numUniqueOptionalLayersTheresRoomFor won't be modified anymore
		int* c = availableOptionalLayers; //< required as stack-array does not automatically create pointer as well?
		//if(test_instance_layers(stderr, numUniqueRequiredLayers, uniqueRequiredLayers, numUniqueOptionalLayersTheresRoomFor, &availableOptionalLayers, &numAvailableOptionalLayers) == 0)
		if(test_instance_layers(stderr, numUniqueRequiredLayers, uniqueRequiredLayers, numUniqueOptionalLayersTheresRoomFor, &c, &numAvailableOptionalLayers) == 0)
		{
			break;
		}

		/*
		// sort (move_to_back) available optional layers
		int* d = availableOptionalLayers;
		int e = -1; //< query for move_to_back
		int f; //< unused (# elements moved to back)
		move_to_back2(NULL, numAvailableOptionalLayers, &d, &e, &f);
		*/

		// bIsAvailablePerInstanceLayer for vulkan_t..
		for(int i = 0; i < NUM_VULKAN_INSTANCE_LAYERS; ++i)
		{
			bIsAvailablePerInstanceLayer[i] = 0;
		}
		for(int i = 0; i < numUniqueRequiredLayers; ++i)
		{
			if(uniqueRequiredLayers[i] == -1)
			{
				continue;
			}

			int layer = uniqueRequiredLayers[i];
			bIsAvailablePerInstanceLayer[layer] = 1;
		}
		for(int i = 0; i < numAvailableOptionalLayers; ++i)
		{
			if(availableOptionalLayers[i] == -1)
			{
				continue;
			}
			
			int layer = availableOptionalLayers[i];
			bIsAvailablePerInstanceLayer[layer] = 1;
		}

		// numLayers and layerNamePerLayer for passing to VkCreateInstance..
		// .. via vkInstanceCreateInfo
		int numLayers = numUniqueRequiredLayers + numAvailableOptionalLayers;
		char* layerNamePerLayer[numLayers];
		int g = 0;
		for(int i = 0; i < numUniqueRequiredLayers; ++i)
		{
			if(uniqueRequiredLayers[i] == -1)
			{
				continue;
			}

			int layer = uniqueRequiredLayers[i];
			layerNamePerLayer[i] = layerNamePerVulkanInstanceLayer[layer];
		}
		g += numUniqueRequiredLayers;
		for(int i = 0; i < numAvailableOptionalLayers; ++i)
		{
			if(availableOptionalLayers[i] == -1)
			{
				continue;
			}

			int layer = availableOptionalLayers[i];
			layerNamePerLayer[g + i] = layerNamePerVulkanInstanceLayer[layer];
		}
		//g += numAvailableOptionalLayers;

		// only numRequiredLayers and requiredLayers from here
		// ^
		// well there is no "other variable" for these as there are..
		// .. numUniqueRequiredLayers and uniqueRequiredLayers for layers,..
		// .. but added the note here anyway for consistency
		
		// for consistency (and thus readability) with above..
		int numAvailableOptionalExtensions = numOptionalExtensions;
		int availableOptionalExtensions[numAvailableOptionalExtensions];
		memcpy(availableOptionalExtensions, optionalExtensions, sizeof(int) * numOptionalExtensions);

		// only numAvailableOptionalExtensions and availableOptionalExtensions from here

		// test instance extensions
		int numOptionalExtensionsTheresRoomFor = numOptionalExtensions;
		int* h = availableOptionalExtensions;
		//if(test_instance_extensions(stderr, bIsAvailablePerInstanceLayer, numRequiredExtensions, requiredExtensions, numOptionalExtensionsTheresRoomFor, &availableOptionalExtensions, &numAvailableOptionalExtensions) != 1)
		if(test_instance_extensions(stderr, bIsAvailablePerInstanceLayer, numRequiredExtensions, requiredExtensions, numOptionalExtensionsTheresRoomFor, &h, &numAvailableOptionalExtensions) != 1)
		{
			break;
		}

		/*
		// sort (move_to_back) available optional extensions
		int* k = availableOptionalExtensions;
		int l = -1; //< query for move_to_back
		int m; //< unused
		move_to_back2(NULL, numAvailableOptionalExtensions, &k, &l, &m);
		*/

		// bIsAvailablePerInstanceExtension for vulkan_t..
		for(int i = 0; i < NUM_VULKAN_INSTANCE_EXTENSIONS; ++i)
		{
			bIsAvailablePerInstanceExtension[i] = 0;
		}
		for(int i = 0; i < numRequiredExtensions; ++i)
		{
			if(requiredExtensions[i] == -1)
			{
				continue;
			}

			int extension = requiredExtensions[i];
			bIsAvailablePerInstanceExtension[extension] = 1;
		}
		for(int i = 0; i < numAvailableOptionalExtensions; ++i)
		{
			if(availableOptionalExtensions[i] == -1)
			{
				continue;
			}

			int extension = availableOptionalExtensions[i];
			bIsAvailablePerInstanceExtension[extension] = 1;
		}

		// numExtensions and extensionNamePerExtension for passing to..
		// .. VkCreateInstance via vkInstanceCreateInfo
		int numExtensions = numRequiredExtensions + numAvailableOptionalExtensions;
		char* extensionNamePerExtension[numExtensions];
		int n = 0;
		for(int i = 0; i < numRequiredExtensions; ++i)
		{
			if(requiredExtensions[i] == -1)
			{
				continue;
			}

			int extension = requiredExtensions[i];
			extensionNamePerExtension[i] = perVulkanInstanceExtension[extension].extensionName;
		}
		n += numRequiredExtensions;
		for(int i = 0; i < numAvailableOptionalExtensions; ++i)
		{
			if(availableOptionalExtensions[i] == -1)
			{
				continue;
			}

			int extension = availableOptionalExtensions[i];
			extensionNamePerExtension[n + i] = perVulkanInstanceExtension[extension].extensionName;
		}
		//n += numAvailableOptionalExtensions;
		
		// create instance
		struct
		{
			VkApplicationInfo a;
		} vkapplicationinfo;
		
		vkapplicationinfo.a = VK_APPLICATION_INFO_DEFAULT;
		//printf("apiVersion %i.%i.%i\n", VK_API_VERSION_MAJOR(vulkan.apiVersion), VK_API_VERSION_MINOR(vulkan.apiVersion), VK_API_VERSION_PATCH(vulkan.apiVersion));
		vkapplicationinfo.a.apiVersion = vulkan.apiVersion;
		
		struct
		{
			VkInstanceCreateInfo a;
		} vkinstancecreateinfo;
		
		vkinstancecreateinfo.a = VK_INSTANCE_CREATE_INFO_DEFAULT;
		vkinstancecreateinfo.a.pApplicationInfo = &vkapplicationinfo.a;
		
		on_printf2(stdout, "numLayers %i\n", numLayers);
		if(numLayers > 0)
		{
			if(on_print != NULL)
			{
				for(int i = 0; i < numLayers; ++i)
				{
					on_printf(stdout, "layerNamePerLayer[%i] %s\n", i, layerNamePerLayer[i]);
				}
			}
		
			vkinstancecreateinfo.a.enabledLayerCount = numLayers;
			//vkinstancecreateinfo.a.ppEnabledLayerNames = layerNamePerLayer;
			vkinstancecreateinfo.a.ppEnabledLayerNames = (const char* const*)layerNamePerLayer;
		}
		on_printf2(stdout, "numExtensions %i\n", numExtensions);
		if(numExtensions > 0)
		{
			if(on_print != NULL)
			{
				for(int i = 0; i < numExtensions; ++i)
				{
					on_printf(stdout, "extensionNamePerExtension[%i] %s\n", i, extensionNamePerExtension[i]);
				}
			}
		
			vkinstancecreateinfo.a.enabledExtensionCount = numExtensions;
			//vkinstancecreateinfo.a.ppEnabledExtensionNames = extensionNamePerExtensions;
			vkinstancecreateinfo.a.ppEnabledExtensionNames = (const char* const*)extensionNamePerExtension;
		}

		VkStructure* o = (VkStructure*)&vkinstancecreateinfo.a;

		int bIsVkkhrdebugutilsExtensionAvailable = bIsAvailablePerInstanceExtension[EVulkanInstanceExtension_Vkkhrdebugutils];
		// ^
		// suffixed with Extension as seems to be no "guarantee that a layer..
		// .. and extension cannot both have the same name"?
		
		struct
		{
			VkDebugUtilsMessengerCreateInfoEXT a;
		} vkdebugutilsmessengercreateinfoext;
		if(bIsVkkhrdebugutilsExtensionAvailable == 1)
		{
			vkdebugutilsmessengercreateinfoext.a = VK_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT_DEFAULT;
			vkdebugutilsmessengercreateinfoext.a.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			vkdebugutilsmessengercreateinfoext.a.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			vkdebugutilsmessengercreateinfoext.a.pfnUserCallback = &myVkDebugUtilsMessengerCallbackEXT;

			o->pNext = &vkdebugutilsmessengercreateinfoext.a;
			o = (VkStructure*)&vkdebugutilsmessengercreateinfoext.a;
		}

		if(vkCreateInstance(&vkinstancecreateinfo.a, NULL, &instance.vkinstance.a) != VK_SUCCESS)
		{
			on_printf2(stderr, "error: vkCreateInstance != VK_SUCCESS in %s\n", __FUNCTION__);
			
			bSuccess = 0;
			break;
		}

		// instance extensions is still part of instance
		if(bIsVkkhrdebugutilsExtensionAvailable == 1)
		{
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance.vkinstance.a, "vkCreateDebugUtilsMessengerEXT");
			if(vkCreateDebugUtilsMessengerEXT == NULL)
			{
				on_printf2(stderr, "error: vkGetInstanceProcAddr == NULL in %s\n", __FUNCTION__);
				
				bSuccess = 0;
				break;
			}
			
			if(vkCreateDebugUtilsMessengerEXT(instance.vkinstance.a, &vkdebugutilsmessengercreateinfoext.a, NULL, &instance.vkdebugutilsmessengerext.a) != VK_SUCCESS)
			{
				on_printf2(stderr, "error: vkCreateDebugUtilsMessengerEXT != VK_SUCCESS in %s\n", __FUNCTION__);
				
				bSuccess = 0;
				break;
			}
		}
	} while(0);
	if(numRequiredLayers > 0)
	{
		remove_elements2(&numRequiredLayers, &requiredLayers);
	}
	if(numOptionalLayers > 0)
	{
		remove_elements2(&numOptionalLayers, &optionalLayers);
	}
	if(numRequiredExtensions > 0)
	{
		remove_elements2(&numRequiredExtensions, &requiredExtensions);
	}
	if(numOptionalExtensions > 0)
	{
		remove_elements2(&numOptionalExtensions, &optionalExtensions);
	}
	if(bSuccess == 0)
	{
		// no "ELoadVkinstanceProgress" because VK_NULL_HANDLE approach here..
		// .. suffices
		// v
		if(instance.vkinstance.a != VK_NULL_HANDLE)
		{
			unload_vkinstance(&instance);
			
			return 0;
		}
	}
	
	//vulkan.instance = instance;
	memcpy(&vulkan.instance, &instance, sizeof(struct vulkan_instance_t));
	memcpy(vulkan.bIsEnabledPerInstanceLayer, bIsAvailablePerInstanceLayer, sizeof(int) * NUM_VULKAN_INSTANCE_LAYERS);
	memcpy(vulkan.bIsEnabledPerInstanceExtension, bIsAvailablePerInstanceExtension, sizeof(int) * NUM_VULKAN_INSTANCE_EXTENSIONS);
	
	return 1;
}

static void unload_vkinstance(struct vulkan_instance_t* instance)
{
	//if((vulkan.flags & EGMLoadVulkanParametersFlag_Safety) != 0)
	//{
		if(instance->vkdebugutilsmessengerext.a != VK_NULL_HANDLE)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->vkinstance.a, "vkDestroyDebugUtilsMessengerEXT");
			if(vkDestroyDebugUtilsMessengerEXT == NULL)
			{
				on_printf2(stdout, "warning: vkGetInstanceProcAddr == NULL in %s\n", __FUNCTION__);
			}
			else
			{
				vkDestroyDebugUtilsMessengerEXT(instance->vkinstance.a, instance->vkdebugutilsmessengerext.a, NULL);
			}
		}
	//}

	vkDestroyInstance(instance->vkinstance.a, NULL);
}
int gm_unload_vkinstance()
{
	if(bIsVulkanLoaded == 0)
	{
		return -1;
	}
	
	if(vulkan.instance.vkinstance.a == VK_NULL_HANDLE)
	{
		return -1;
	}
	
	unload_vkinstance(&vulkan.instance);
	
	//vulkan.instance = vulkan_instance_default;
	memcpy(&vulkan.instance, &vulkan_instance_default, sizeof(struct vulkan_instance_t));
	
	return 1;
}

int gm_get_info_about_vulkan(struct gm_info_about_vulkan_t* infoAboutVulkan)
{
	if(bIsVulkanLoaded == 0)
	{
		return -1;
	}
	
	infoAboutVulkan->apiVersion = vulkan.apiVersion;
	infoAboutVulkan->vkinstance.a = vulkan.instance.vkinstance.a;
	//infoAboutVulkan->vkdevice.a = vulkan.device.vkdevice.a;
	
	return 1;
}

//*****************************************************************************
//                                  directx11
//*****************************************************************************

#define IUNKNOWN_RELEASE(a) IUnknown_Release((IUnknown*)a)

D3D_FEATURE_LEVEL D3D_FEATURE_LEVELS[] = {
	D3D_FEATURE_LEVEL_9_1,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_11_1
};

#define D3D_FEATURE_LEVELS_LENGTH (sizeof D3D_FEATURE_LEVELS / sizeof *D3D_FEATURE_LEVELS)

//*****************************************************************************

static int bIsDirectx11Loaded = 0;

struct directx11_t
{
	struct
	{
		ID3D11Device* a;
	} id3d11device;
	struct
	{
		ID3D11DeviceContext* a;
	} id3d11devicecontext;
};
static const struct directx11_t directx11_default = {
		//...
	};

static struct directx11_t directx11 = directx11_default;

//*****************************************************************************

static void unload_directx11(int progress, struct directx11_t* a);
enum
{
	ELoadDirectx11Progress_CreateDXGIFactory = 1,
	ELoadDirectx11Progress_Idxgiadapter,
	ELoadDirectx11Progress_D3D11CreateDevice
};
#define ELoadDirectx11Progress_All ELoadDirectx11Progress_D3D11CreateDevice
int gm_load_directx11(struct gm_load_directx11_parameters_t* parameters)
{
	if(bIsDirectx11Loaded == 1)
	{
		return -1;
	}

	struct
	{
		HRESULT a;
	} hresult;

	struct
	{
		IDXGIFactory6* a;
	} idxgifactory6;

	struct
	{
		IDXGIAdapter* a;
	} idxgiadapter;
	
	struct
	{
		ID3D11Device* a;
	} id3d11device;

	struct
	{
		ID3D11DeviceContext* a;
	} id3d11devicecontext;

	int progress = 0;

	do
	{
		// idxgifactory..
		hresult.a = CreateDXGIFactory(&IID_IDXGIFactory6, (void**)&idxgifactory6.a);
		if(FAILED(hresult.a))
		{
			if(on_print != NULL)
			{
				int a;
				get_hresult_to_string(hresult.a, &a, NULL);
				char b[a];
				get_hresult_to_string(hresult.a, &a, b);

				on_printf(stderr, "error: %s in %s\n", b, __FUNCTION__);
			}
			break;
		}
		progress = ELoadDirectx11Progress_CreateDXGIFactory;

		int numAdapters;
		for(numAdapters = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory6_EnumAdapterByGpuPreference(idxgifactory6.a, numAdapters, parameters->gpuPreference, &IID_IDXGIAdapter, (void**)&idxgiadapter.a); ++numAdapters)
		{
			IUNKNOWN_RELEASE(idxgiadapter.a);
		}
		if(numAdapters == 0)
		{
			on_printf2(stderr, "error: no adapters found in %s\n", __FUNCTION__);
			break;
		}

		//printf("numAdapters is %i\n", numAdapters);

		//IDXGIAdapter* adapters[numAdapters];
		for(int i = 0; i < numAdapters; ++i)
		{
			IDXGIFactory6_EnumAdapterByGpuPreference(idxgifactory6.a, i, parameters->gpuPreference, &IID_IDXGIAdapter, (void**)&idxgiadapter.a);

			int numOutputs;
			struct
			{
				IDXGIOutput* a;
			} idxgioutput;
			//printf("i is %i\n", i);
			for(numOutputs = 0; DXGI_ERROR_NOT_FOUND != IDXGIAdapter_EnumOutputs(idxgiadapter.a, numOutputs, &idxgioutput.a); ++numOutputs)
			{
				IUNKNOWN_RELEASE(idxgioutput.a);
			}
			//printf("numOutputs is %i\n", numOutputs);
			
			/*
			if(numOutputs == 0)
			{
				adapters[i] = NULL;
			}
			*/
			if(numOutputs > 0)
			{
				break;
			}
			if(i == numAdapters - 1)
			{
				idxgiadapter.a = NULL;
			}
		}
		if(idxgiadapter.a == NULL)
		{
			on_printf2(stderr, "error: no output on any adapter in %s\n", __FUNCTION__);
			break;
		}
		progress = ELoadDirectx11Progress_Idxgiadapter;

		// id3d11device and id3d11devicecontext..
		D3D_FEATURE_LEVEL* featureLevels;
		int numFeatureLevels;

		int indexToMinFeatureLevel = -1;
		D3D_FEATURE_LEVEL minFeatureLevel;
		for(int i = 0; i < D3D_FEATURE_LEVELS_LENGTH; ++i)
		{
			if(parameters->minFeatureLevel == D3D_FEATURE_LEVELS[i])
			{
				featureLevels = &D3D_FEATURE_LEVELS[i];
				indexToMinFeatureLevel = i;
			}
			if(parameters->maxFeatureLevel == D3D_FEATURE_LEVELS[i])
			{
				numFeatureLevels = 1 + (i - indexToMinFeatureLevel);
			}
		}

		struct
		{
			DXGI_ADAPTER_DESC a;
		} dxgiadapterdesc;
		IDXGIAdapter_GetDesc(idxgiadapter.a, &dxgiadapterdesc.a);
		on_printf2(stdout, "adapter %ls chosen in %s\n", dxgiadapterdesc.a.Description, __FUNCTION__);

		UINT flags = 0;
		if((parameters->flags & EGMLoadDirectx11ParametersFlag_Safety) != 0)
		{
			flags |= D3D11_CREATE_DEVICE_DEBUG;
		}
		hresult.a = D3D11CreateDevice(idxgiadapter.a, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &id3d11device.a, NULL, &id3d11devicecontext.a);
		if(FAILED(hresult.a))
		{
			if(on_print != NULL)
			{
				int a;
				get_hresult_to_string(hresult.a, &a, NULL);
				char b[a];
				get_hresult_to_string(hresult.a, &a, b);

				on_printf(stderr, "error: %s in %s\n", b, __FUNCTION__);
			}
			break;
		}
		progress = ELoadDirectx11Progress_D3D11CreateDevice;
	} while(0);
	// always release..
	if(progress >= ELoadDirectx11Progress_Idxgiadapter)
	{
		IUNKNOWN_RELEASE(idxgiadapter.a);
	}
	if(progress >= ELoadDirectx11Progress_CreateDXGIFactory)
	{
		IUNKNOWN_RELEASE(idxgifactory6.a);
	}
	// only release if failed..
	if(progress < ELoadDirectx11Progress_All)
	{
		struct directx11_t a;
		a.id3d11device.a = id3d11device.a;
		a.id3d11devicecontext.a = id3d11devicecontext.a;
		unload_directx11(progress, &a);

		return 0;
	}

	directx11.id3d11device.a = id3d11device.a;
	directx11.id3d11devicecontext.a = id3d11devicecontext.a;

	return 1;
}
static void unload_directx11(int progress, struct directx11_t* a)
{
	if(progress >= ELoadDirectx11Progress_D3D11CreateDevice)
	{
		IUNKNOWN_RELEASE(a->id3d11devicecontext.a);
		IUNKNOWN_RELEASE(a->id3d11device.a);
	}
}
int gm_unload_directx11()
{
	if(bIsDirectx11Loaded == 0)
	{
		return -1;
	}

	unload_directx11(ELoadDirectx11Progress_All, &directx11);

	return 1;
};
