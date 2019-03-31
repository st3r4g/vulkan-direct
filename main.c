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

int main() {
	uint32_t apiVersion;
	vkEnumerateInstanceVersion(&apiVersion);
	printf("Vulkan %i.%i.%i\n", VK_VERSION_MAJOR(apiVersion),
	VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
// I should check the availability of extensions here

	const char *ext[] = {"VK_KHR_display"};
	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledExtensionCount = 1;
	createInfo.ppEnabledExtensionNames = ext;
	VkInstance instance;
	vkCreateInstance(&createInfo, 0, &instance);

	VkPhysicalDevice gpu = get_gpu(instance);

	VkDisplayPropertiesKHR displayProperties = get_display(gpu);


//	vkGetPhysicalDeviceDisplayPlanePropertiesKHR()
//	vkGetDisplayPlaneSupportedDisplaysKHR()
//	vkGetDisplayModePropertiesKHR()
//	vkCreateDisplayPlaneSurfaceKHR();
//	vkCreateImageView();
//	vkCreateSwapchainKHR();
}
