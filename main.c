#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <vulkan/vulkan.h>

#define maxPhysicalDeviceCount 4
VkPhysicalDevice get_gpu(VkInstance instance) {
	VkPhysicalDevice physicalDevices[maxPhysicalDeviceCount];
	uint32_t physicalDeviceCount = maxPhysicalDeviceCount;

	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);

	printf("List of available GPUs:\n");
	for (int i=0; i<physicalDeviceCount; i++) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
		printf("  %s [Vulkan %i.%i.%i]\n", properties.deviceName,
		VK_VERSION_MAJOR(properties.apiVersion),
		VK_VERSION_MINOR(properties.apiVersion),
		VK_VERSION_PATCH(properties.apiVersion));
	}

	printf("Using the first GPU in the list\n");
	return physicalDevices[0];
}
#undef maxPhysicalDeviceCount

#define maxDisplayCount 4
VkDisplayPropertiesKHR get_display(VkPhysicalDevice physicalDevice) {
	VkDisplayPropertiesKHR displayProperties[maxDisplayCount];
	uint32_t displayCount = maxDisplayCount;

	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayCount, displayProperties);

	printf("List of available displays for the selected GPU:\n");
	for (int i=0; i<displayCount; i++) {
// WARNING: displayName can be NULL
		printf("  %s [%ix%i]\n", displayProperties[i].displayName,
		displayProperties[i].physicalResolution.width,
		displayProperties[i].physicalResolution.height);
	}

	printf("Using the first display in the list\n");
	return displayProperties[0];
}
#undef maxDisplayCount

VkSurfaceKHR create_surface(VkInstance inst, VkPhysicalDevice pdev,
VkDisplayKHR disp) {
	uint32_t count = 1;
	VkDisplayModePropertiesKHR props;
	vkGetDisplayModePropertiesKHR(pdev, disp, &count, &props);

	VkDisplaySurfaceCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
		.displayMode = props.displayMode,
		.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
		.imageExtent = props.parameters.visibleRegion
	};

	VkSurfaceKHR surf;
	if(vkCreateDisplayPlaneSurfaceKHR(inst, &info, NULL, &surf))
		return VK_NULL_HANDLE;
	return surf;
}

VkDevice create_logical_device(VkPhysicalDevice pdev) {
	VkDevice dev;
	VkDeviceCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkDeviceQueueCreateInfo info_ = {0};
	info_.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info_.queueFamilyIndex = 0; // ASSUMPTION: has GRAPHICS flag
	info_.queueCount = 1;
	float priority = 1.0f;
	info_.pQueuePriorities = &priority;

	info.queueCreateInfoCount = 1;
	info.pQueueCreateInfos = &info_;
	const char *extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	info.enabledExtensionCount = sizeof(extensions)/sizeof(char*);
	info.ppEnabledExtensionNames = extensions;

	if (vkCreateDevice(pdev, &info, NULL, &dev) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return dev;
}

VkSwapchainKHR create_swapchain(VkPhysicalDevice pdev, VkDevice dev, VkSurfaceKHR surf) {
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &caps);
	uint32_t count = 64;
	VkPresentModeKHR props[64];
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &count, props);
	for (int i=0; i<count; i++) {
		printf("%d %d\n", caps.currentExtent.width,
		caps.currentExtent.height);
	}

	VkSwapchainCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surf,
		.minImageCount = 3,
		.imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = caps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	VkSwapchainKHR swp;
	if (vkCreateSwapchainKHR(dev, &info, NULL, &swp) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return swp;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT messageType, const
VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

int main() {
	uint32_t apiVersion;
	vkEnumerateInstanceVersion(&apiVersion);
	printf("Vulkan %i.%i.%i\n", VK_VERSION_MAJOR(apiVersion),
	VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
// I should check the availability of extensions here

	uint32_t propertyCount = 64;
	VkLayerProperties props[64];
	vkEnumerateInstanceLayerProperties(&propertyCount, props);
	for (int i=0; i<propertyCount; i++)
		printf("%s\n", props[i].layerName);

	VkDebugUtilsMessengerCreateInfoEXT createInfo2 = {0};
	createInfo2.sType =
	VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo2.messageSeverity =
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo2.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo2.pfnUserCallback = debugCallback;

	const char *extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_DISPLAY_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	const char *layers[] = {"VK_LAYER_LUNARG_standard_validation"};
	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = &createInfo2;
	createInfo.enabledLayerCount = sizeof(layers)/sizeof(char*);
	createInfo.ppEnabledLayerNames = layers;
	createInfo.enabledExtensionCount = sizeof(extensions)/sizeof(char*);
	createInfo.ppEnabledExtensionNames = extensions;
	VkInstance instance;
	vkCreateInstance(&createInfo, 0, &instance);

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
	(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
	"vkCreateDebugUtilsMessengerEXT");
	VkDebugUtilsMessengerEXT debugMessenger;
//	vkCreateDebugUtilsMessenger(instance, &createInfo2, 0, &debugMessenger);

	VkPhysicalDevice gpu = get_gpu(instance);
	VkDevice dev = create_logical_device(gpu);
	VkQueue queue;
	vkGetDeviceQueue(dev, 0, 0, &queue);

	VkDisplayPropertiesKHR displayProperties = get_display(gpu);
	VkSurfaceKHR surf = create_surface(instance, gpu, displayProperties.display);

	VkBool32 supp;
	vkGetPhysicalDeviceSurfaceSupportKHR(gpu, 0, surf, &supp);
	printf("supported? %d\n", supp);

	VkSwapchainKHR swp = create_swapchain(gpu, dev, surf);
	uint32_t imageCount = 4;
	VkImage imgs[4];
	vkGetSwapchainImagesKHR(dev, swp, &imageCount, imgs);

/*	VkImageView = view;
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = view;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = swapChainImageFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(dev, &createInfo, NULL, &view);*/

	VkImageSubresourceRange aaa = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	VkClearColorValue color = {0.8984375f, 0.8984375f, 0.9765625f, 1.0f};

	VkCommandPool commandPool;
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = 0;
	vkCreateCommandPool(dev, &poolInfo, NULL, &commandPool);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer cmdbuf;
	vkAllocateCommandBuffers(dev, &allocInfo, &cmdbuf);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	vkBeginCommandBuffer(cmdbuf, &beginInfo);

	VkImageMemoryBarrier bbb = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.image = imgs[0],
		.subresourceRange = aaa
	};
	vkCmdPipelineBarrier(cmdbuf, 1, 1, 0, 0, 0, 0, 0, 1, &bbb);
	vkCmdClearColorImage(cmdbuf, imgs[0],
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &aaa);
	bbb.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(cmdbuf, 1, 1, 0, 0, 0, 0, 0, 1, &bbb);

	vkEndCommandBuffer(cmdbuf);

	uint32_t index;
	vkAcquireNextImageKHR(dev, swp, UINT64_MAX, VK_NULL_HANDLE,
	VK_NULL_HANDLE, &index);


VkSubmitInfo submitInfo = {};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
//VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//submitInfo.waitSemaphoreCount = 1;
//submitInfo.pWaitSemaphores = waitSemaphores;
//submitInfo.pWaitDstStageMask = waitStages;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &cmdbuf;
//VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
//submitInfo.signalSemaphoreCount = 1;
//submitInfo.pSignalSemaphores = signalSemaphores;
vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &swp,
		.pImageIndices = &index
	};
	vkQueuePresentKHR(queue, &presentInfo);
	sleep(3);
	
	vkDestroyCommandPool(dev, commandPool, NULL);
	vkDestroySwapchainKHR(dev, swp, 0);
	vkDestroySurfaceKHR(instance, surf, 0);
	vkDestroyDevice(dev, 0);

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger =
	(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
	"vkDestroyDebugUtilsMessengerEXT");
//	vkDestroyDebugUtilsMessenger(instance, debugMessenger, 0);
	vkDestroyInstance(instance, 0);

	return 0;
}
