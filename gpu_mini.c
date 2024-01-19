#include "gpu_mini.h"


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
};
static const struct vulkan_t vulkan_default = {
		.instance = vulkan_instance_default
	};

static struct vulkan_t vulkan = vulkan_default;

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
//       layerNamePer*Layer[..] may == NULL to indicate a gap
//       if succeeded.. will write to *numOptionalLayersAvailable
// NOTE: no such as "instance layer" as "Since version 1.0.13 of the Vulkan..
//       .. API specification [...] the Vulkan SDK device layers have been..
//       .. deprecated"
//       ^
//       https://gpuopen.com/learn/using-the-vulkan-validation-layers/
static int test_instance_layers(FILE* a, int numRequiredLayersTheresRoomFor, char** layerNamePerRequiredLayer, int numOptionalLayersTheresRoomFor, char** layerNamePerOptionalLayer, int* numOptionalLayersAvailable)
{
	struct
	{
		FILE* a;
	} file; //< "errormost" file
	if(numRequiredLayersTheresRoomFor > 0)
	{
		file.a = a == stderr ? stderr : stdout;
	}
	else
	{
		file.a = stdout;
	}
	
	uint32_t b;
	if(vkEnumerateInstanceLayerProperties(&b, NULL) != VK_SUCCESS)
	{
		on_printf2(file.a, "%s: vkEnumerateInstanceLayerProperties != VK_SUCCESS in %s\n", file.a == stderr ? "error" : "warning", __FUNCTION__);
		return file.a == stderr ? 0 : 1;
	}
	
	VkLayerProperties c[b];
	
	if(vkEnumerateInstanceLayerProperties(&b, c) != VK_SUCCESS)
	{
		on_printf2(file.a, "%s: vkEnumerateInstanceLayerProperties != VK_SUCCESS in %s\n", file.a == stderr ? "error" : "warning", __FUNCTION__);
		return file.a == stderr ? 0 : 1;
	}
	
	int bIsAnyRequiredLayerNotAvailable = 0;
	for(int i = 0; i < numRequiredLayersTheresRoomFor; ++i)
	{
		if(layerNamePerRequiredLayer[i] == NULL)
		{
			continue; //< ignore gap
		}
	
		char* layerNameForRequiredLayer = layerNamePerRequiredLayer[i];

		int bIsLayerAvailable = 0;
		for(int j = 0; j < b; ++j)
		{
			if(strcmp(layerNameForRequiredLayer, c[j].layerName) == 0)
			{
				bIsLayerAvailable = 1;
				break;
			}
		}
		if(bIsLayerAvailable == 0)
		{
			return 0;
		}
	}
	
	if(numOptionalLayersTheresRoomFor > 0)
	{
		int d = 0;
		for(int i = 0; i < numOptionalLayersTheresRoomFor; ++i)
		{
			char* layerNameForOptionalLayer = layerNamePerOptionalLayer[i];
			if(layerNameForOptionalLayer == NULL)
			{
				continue; //< ignore gap
			}

			int bIsLayerAvailable = 0;
			for(int j = 0; j < b; ++j)
			{
				if(strcmp(layerNameForOptionalLayer, c[j].layerName) == 0)
				{
					bIsLayerAvailable = 1;
					++d;
					
					break;
				}
			}

			if(bIsLayerAvailable == 0)
			{
				layerNamePerOptionalLayer[i] = NULL;
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
	uint32_t b;
	// "[If] pLayerName parameter is NULL, only extensions provided by..
	// .. the Vulkan implementation or by implicitly enabled layers are..
	// .. returned",
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkEnumerateInstanceExtensionProperties.html
	// v
	if(vkEnumerateInstanceExtensionProperties(layerName, &b, NULL) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceExtensionProperties != VK_SUCCESS in %s\n", a == stderr ? "error" : "warning", __FUNCTION__); 
		return 0;
	}
	
	VkExtensionProperties c[b];
	
	if(vkEnumerateInstanceExtensionProperties(layerName, &b, c) != VK_SUCCESS)
	{
		on_printf2(a, "%s: vkEnumerateInstanceExtensionProperties != VK_SUCCESS in %s\n", a == stderr ? "error" : "warning", __FUNCTION__);
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
	char* extensionName;
};
// NOTE: returns 1 if succeeded
//       returns 0 if failed
// NOTE: if a == stderr.. print error
//       if a == stdout.. print warning
//       perRequiredExtension[..].layerName may == -1 to indicate layerName == NULL
//       perOptionalExtension[..].layerName may == -1 to indicate layerName == NULL
//       perRequiredExtension[..].extensionName may == NULL to indicate a gap
//       perOptionalExtension[..].extensionName may == NULL to indicate a gap
//       if succeeded..
//       .. if an optional extension at index .. is not available will set..
//          .. perOptionalExtension[..].extensionName == NULL
//       .. will write to *numOptionalExtensionsAvailable
// NOTE: assumes that each layer if any is tested even for optional layer(s)..
//       .. if any
static int test_instance_extensions(FILE* a, int numRequiredExtensionsTheresRoomFor, struct test_instance_extensions_per_extension_t* perRequiredExtension, int numOptionalExtensionsTheresRoomFor, struct test_instance_extensions_per_extension_t* perOptionalExtension, int* numOptionalExtensionsAvailable)
{
	struct
	{
		FILE* a;
	} file; //< "errormost" file
	if(numRequiredExtensionsTheresRoomFor > 0)
	{
		file.a = a == stderr ? stderr : stdout;
	}
	else
	{
		file.a = stdout;
	}

	for(int i = 0; i < numRequiredExtensionsTheresRoomFor; ++i)
	{
		int layer = perRequiredExtension[i].layer;
		char* layerName = layerNamePerVulkanInstanceLayer[layer];
		char* extensionName = perRequiredExtension[i].extensionName;
		if(extensionName == NULL)
		{
			continue; //< ignore gap
		}
		
		if(test_instance_extension(file.a, layerName, extensionName) == 0)
		{
			// only print warning about external library call, thus in..
			// .. test_instance_extension..
			// .. vkEnumerateInstanceExtensionProperties warning is fine,..
			// .. but don't print warning about test_instance_extension fail
			// v
			if(file.a == stderr)
			{
				on_printf2(stderr, "error: required instance extension %s is missing (layerName == %s) in %s\n", extensionName, layerName, __FUNCTION__);
			}
			return 0;
		}
	}

	if(numOptionalExtensionsTheresRoomFor != 0)
	{
		int b = 0;
		for(int i = 0; i < numOptionalExtensionsTheresRoomFor; ++i)
		{
			int layer = perOptionalExtension[i].layer;
			char* layerName = layerNamePerVulkanInstanceLayer[layer];
			char* extensionName = perOptionalExtension[i].extensionName;
			if(extensionName == NULL)
			{
				continue; //< ignore gap
			}
			
			//            optional instance extension should never print error
			//                         v
			if(test_instance_extension(stdout, layerName, extensionName) == 0)
			{
				perOptionalExtension[i].extensionName = NULL;
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
	
	int bSuccess = 1;
	do
	{
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
			int f = EVulkanInstanceExtension_Vkkhrdebugutils;
			add_or_append_element2(&numOptionalExtensions, &optionalExtensions, &f);
		}
		
		// test instance layers
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
		
		int numUniqueOptionalLayers;
		int indexPerUniqueOptionalLayer[numOptionalLayers];
		//get_uniques2(NULL, numOptionalLayers, optionalLayers, &numUniqueOptionalLayers, &indexPerUniqueOptionalLayer);
		int* b = indexPerUniqueOptionalLayer;
		get_uniques2(NULL, numOptionalLayers, optionalLayers, &numUniqueOptionalLayers, &b);
		int uniqueOptionalLayers[numUniqueOptionalLayers];
		for(int i = 0; i < numUniqueOptionalLayers; ++i)
		{
			uniqueOptionalLayers[i] = optionalLayers[indexPerUniqueOptionalLayer[i]];
		}
		
		char* layerNamePerUniqueRequiredLayer[numUniqueRequiredLayers];
		for(int i = 0; i < numUniqueRequiredLayers; ++i)
		{
			layerNamePerUniqueRequiredLayer[i] = layerNamePerVulkanInstanceLayer[uniqueRequiredLayers[i]];
		}
		
		char* layerNamePerUniqueOptionalLayer[numUniqueOptionalLayers];
		for(int i = 0; i < numUniqueOptionalLayers; ++i)
		{
			layerNamePerUniqueOptionalLayer[i] = layerNamePerVulkanInstanceLayer[uniqueOptionalLayers[i]];
		}
		
		int numOptionalLayersAvailable;
		//if(test_instance_layers(stderr, numUniqueRequiredLayers, layerNamePerUniqueRequiredLayer, numUniqueOptionalLayers, &layerNamePerUniqueOptionalLayer, &numOptionalLayersAvailable) == 0)
		if(test_instance_layers(stderr, numUniqueRequiredLayers, layerNamePerUniqueRequiredLayer, numUniqueOptionalLayers, layerNamePerUniqueOptionalLayer, &numOptionalLayersAvailable) == 0)
		{
			break;
		}
		
		char* layerNamePerAvailableOptionalLayer[numUniqueOptionalLayers];
		memcpy(layerNamePerAvailableOptionalLayer, layerNamePerUniqueOptionalLayer, sizeof(char*) * numUniqueOptionalLayers);
		int c;
		char* d = NULL; //< query for move_to_back
		//move_to_back2(NULL, numOptionalLayersAvailable, layerNamePerAvailableOptionalLayer, &d, &c);
		char** e = layerNamePerAvailableOptionalLayer;
		move_to_back2(NULL, numOptionalLayersAvailable, &e, &d, &c);
		
		int numLayers = numUniqueRequiredLayers + numOptionalLayersAvailable;
		char* layerNamePerLayer[numLayers];
		int f = 0;
		memcpy(layerNamePerLayer, layerNamePerUniqueRequiredLayer, sizeof(char*) * numUniqueRequiredLayers);
		f += numUniqueRequiredLayers;
		memcpy(layerNamePerLayer + f, layerNamePerAvailableOptionalLayer, sizeof(char*) * numOptionalLayersAvailable);
		
		// test instance extensions
		struct test_instance_extensions_per_extension_t perRequiredExtension[numRequiredExtensions];
		for(int i = 0; i < numRequiredExtensions; ++i)
		{
			int extension = requiredExtensions[i];
		
			perRequiredExtension[i].layer = perVulkanInstanceExtension[extension].layer;
			perRequiredExtension[i].extensionName = perVulkanInstanceExtension[extension].extensionName;
		}
		
		struct test_instance_extensions_per_extension_t perOptionalExtension[numOptionalExtensions];
		for(int i = 0; i < numOptionalExtensions; ++i)
		{
			int extension = optionalExtensions[i];
			
			perOptionalExtension[i].layer = perVulkanInstanceExtension[extension].layer;
			perOptionalExtension[i].extensionName = perVulkanInstanceExtension[extension].extensionName;
		}
		
		int numOptionalExtensionsAvailable;
		if(test_instance_extensions(stderr, numRequiredExtensions, perRequiredExtension, numOptionalExtensions, perOptionalExtension, &numOptionalExtensionsAvailable) != 1)
		{
			break;
		}
		
		char* extensionNamePerAvailableOptionalExtension[numOptionalExtensions];
		for(int i = 0; i < numOptionalExtensions; ++i)
		{
			extensionNamePerAvailableOptionalExtension[i] = perOptionalExtension[i].extensionName;
		}
		int g;
		char* h = NULL; //< query for move_to_back
		//move_to_back2(NULL, numOptionalExtensions, &extensionNamePerAvailableOptionalExtension, &h, &g);
		char** k = extensionNamePerAvailableOptionalExtension;
		move_to_back2(NULL, numOptionalExtensions, &k, &h, &g);
		
		int numExtensions = numRequiredExtensions + numOptionalExtensionsAvailable;
		char* extensionNamePerExtension[numExtensions];
		int l = 0;
		for(int i = 0; i < numRequiredExtensions; ++i)
		{
			extensionNamePerExtension[i] = perRequiredExtension[i].extensionName;
		}
		l += numRequiredExtensions;
		memcpy(extensionNamePerExtension + l, extensionNamePerAvailableOptionalExtension, sizeof(char*) * numOptionalExtensionsAvailable);
		
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
		if(numLayers > 0)
		{
			/*
			printf("numLayers %i\n", numLayers);
			for(int i = 0; i < numLayers; ++i)
			{
				printf("layerNamePerLayer[%i] %s\n", i, layerNamePerLayer[i]);
			}
			*/
		
			vkinstancecreateinfo.a.enabledLayerCount = numLayers;
			//vkinstancecreateinfo.a.ppEnabledLayerNames = layerNamePerLayer;
			vkinstancecreateinfo.a.ppEnabledLayerNames = (const char* const*)layerNamePerLayer;
		}
		if(numExtensions > 0)
		{
			/*
			printf("numExtensions %i\n", numExtensions);
			for(int i = 0; i < numExtensions; ++i)
			{
				printf("extensionNamePerExtension[%i] %s\n", i, extensionNamePerExtension[i]);
			}
			*/
		
			vkinstancecreateinfo.a.enabledExtensionCount = numExtensions;
			//vkinstancecreateinfo.a.ppEnabledExtensionNames = extensionNamePerExtensions;
			vkinstancecreateinfo.a.ppEnabledExtensionNames = (const char* const*)extensionNamePerExtension;
		}

		VkStructure* m = (VkStructure*)&vkinstancecreateinfo.a;

		// TODO: set vulkan.numLayers and vulkan.numExtensions, which will..
		//       .. allow for the below..
		//int bIsVkkhrdebugutilsExtensionEnabled = is_in2(NULL, numExtensions, extensions, &EVulkanInstanceExtension_Vkkhrdebugutils);
		int bIsVkkhrdebugutilsExtensionEnabled = is_in2(predicate$char$, numExtensions, extensionNamePerExtension, &perVulkanInstanceExtension[EVulkanInstanceExtension_Vkkhrdebugutils].extensionName);
		// ^
		// suffixed with Extension as seems to be no "guarantee that a layer..
		// .. and extension cannot both have the same name"?
		
		struct
		{
			VkDebugUtilsMessengerCreateInfoEXT a;
		} vkdebugutilsmessengercreateinfoext;
		if(bIsVkkhrdebugutilsExtensionEnabled == 1)
		{
			vkdebugutilsmessengercreateinfoext.a = VK_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT_DEFAULT;
			vkdebugutilsmessengercreateinfoext.a.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			vkdebugutilsmessengercreateinfoext.a.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			vkdebugutilsmessengercreateinfoext.a.pfnUserCallback = &myVkDebugUtilsMessengerCallbackEXT;

			m->pNext = &vkdebugutilsmessengercreateinfoext.a;
			m = (VkStructure*)&vkdebugutilsmessengercreateinfoext.a;
		}

		if(vkCreateInstance(&vkinstancecreateinfo.a, NULL, &instance.vkinstance.a) != VK_SUCCESS)
		{
			on_printf2(stderr, "error: vkCreateInstance != VK_SUCCESS in %s\n", __FUNCTION__);
			
			bSuccess = 0;
			break;
		}

		// instance extensions is still part of instance
		if(bIsVkkhrdebugutilsExtensionEnabled == 1)
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
