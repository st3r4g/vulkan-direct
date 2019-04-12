#include <stdio.h>

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

	const char *extensions[] = {
		VK_KHR_DISPLAY_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	const char *layers[] = {"VK_LAYER_LUNARG_standard_validation"};
	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledLayerCount = sizeof(layers)/sizeof(char*);
	createInfo.ppEnabledLayerNames = layers;
	createInfo.enabledExtensionCount = sizeof(extensions)/sizeof(char*);
	createInfo.ppEnabledExtensionNames = extensions;
	VkInstance instance;
	vkCreateInstance(&createInfo, 0, &instance);

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
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
	(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
	"vkCreateDebugUtilsMessengerEXT");
	VkDebugUtilsMessengerEXT debugMessenger;
	vkCreateDebugUtilsMessenger(instance, &createInfo2, 0, &debugMessenger);

	VkPhysicalDevice gpu = get_gpu(instance);

	VkDisplayPropertiesKHR displayProperties = get_display(gpu);


//	vkGetPhysicalDeviceDisplayPlanePropertiesKHR()
//	vkGetDisplayPlaneSupportedDisplaysKHR()
//	vkGetDisplayModePropertiesKHR()
//	vkCreateDisplayPlaneSurfaceKHR();
//	vkCreateImageView();
//	vkCreateSwapchainKHR();


	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger =
	(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
	"vkDestroyDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessenger(instance, debugMessenger, 0);
	vkDestroyInstance(instance, 0);

	return 0;
}
