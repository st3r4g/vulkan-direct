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

int main() {
	uint32_t apiVersion;
	vkEnumerateInstanceVersion(&apiVersion);
	printf("Vulkan %i.%i.%i\n", VK_VERSION_MAJOR(apiVersion),
	VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

	VkInstanceCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	VkInstance instance;
	vkCreateInstance(&createInfo, 0, &instance);

	VkPhysicalDevice gpu = get_gpu(instance);

//	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &propertyCount, 0);
//	vkGetPhysicalDeviceDisplayPlanePropertiesKHR()
//	vkGetDisplayPlaneSupportedDisplaysKHR()
//	vkGetDisplayModePropertiesKHR()
//	vkCreateDisplayPlaneSurfaceKHR();
//	vkCreateImageView();
//	vkCreateSwapchainKHR();
}
