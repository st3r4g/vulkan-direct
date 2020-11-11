#include <vulkan/vulkan.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include <intel_bufmgr.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static PFN_vkGetMemoryFdKHR vkGetMemoryFd = 0;

VkInstance create_instance() {
	const char *extensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME
	};
	const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.enabledLayerCount = sizeof(layers)/sizeof(char*),
		.ppEnabledLayerNames = layers,
		.enabledExtensionCount = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions
	};
	VkInstance inst;
	if (vkCreateInstance(&info, NULL, &inst)) {
		fprintf(stderr, "ERROR: create_instance() failed.\n");
		return VK_NULL_HANDLE;
	}

	vkGetMemoryFd = (PFN_vkGetMemoryFdKHR) vkGetInstanceProcAddr(inst,
	"vkGetMemoryFdKHR");

	return inst;
}

VkPhysicalDevice get_physical_device(VkInstance inst) {
	uint32_t n = 1;
	VkPhysicalDevice pdev;
	vkEnumeratePhysicalDevices(inst, &n, &pdev);
	if (n == 0) {
		fprintf(stderr, "ERROR: get_physical_device() failed.\n");
		return VK_NULL_HANDLE;
	}
	return pdev;
}

VkDevice create_device(VkInstance inst, VkPhysicalDevice pdev) {
	const char *extensions[] = {
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
		VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME
	};
	float priority = 1.0f;
	VkDeviceQueueCreateInfo infoQueue = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	VkDeviceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &infoQueue,
		.enabledExtensionCount = sizeof(extensions)/sizeof(char*),
		.ppEnabledExtensionNames = extensions,
	};
	VkDevice dev;
	if (vkCreateDevice(pdev, &info, NULL, &dev)) {
		fprintf(stderr, "ERROR: create_device() failed.\n");
		return VK_NULL_HANDLE;
	}
	return dev;
}

VkImage create_image(VkDevice dev, uint32_t width, uint32_t height) {
	VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_UNORM, // TODO
		.extent = {width, height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_LINEAR,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // or SRC
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
//		.queueFamilyIndexCount = 0, ignored
//		.pQueueFamilyIndices = 0, ignored
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkImage img;
	if (vkCreateImage(dev, &imageCreateInfo, NULL, &img)) {
		fprintf(stderr, "ERROR: create_image() failed.\n");
		return VK_NULL_HANDLE;
	}
	return img;
}

uint32_t findMemoryType(VkPhysicalDevice pdev, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(pdev, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	return 0;
}

VkDeviceMemory allocate_memory(VkPhysicalDevice pdev, VkDevice dev, VkImage img) {
	VkMemoryRequirements memreq;
	vkGetImageMemoryRequirements(dev, img, &memreq);

	uint32_t index = findMemoryType(pdev, memreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = 0,
		.allocationSize = memreq.size,
		.memoryTypeIndex = index
	};
	VkDeviceMemory mem;
	if (vkAllocateMemory(dev, &memoryAllocateInfo, NULL, &mem)) {
		fprintf(stderr, "ERROR: allocate_memory() failed.\n");
		return VK_NULL_HANDLE;
	}
	return mem;
}

VkCommandPool create_command_pool(VkDevice dev) {
	VkCommandPoolCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	};
	VkCommandPool pool;
	if (vkCreateCommandPool(dev, &info, NULL, &pool)) {
		fprintf(stderr, "ERROR: create_command_pool() failed.\n");
		return VK_NULL_HANDLE;
	}
	return pool;
}

VkCommandBuffer record_command_clear(VkDevice dev, VkCommandPool pool, VkImage img) {
	VkCommandBufferAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer cmdbuf;
	vkAllocateCommandBuffers(dev, &info, &cmdbuf);

	VkCommandBufferBeginInfo infoBegin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	};
	vkBeginCommandBuffer(cmdbuf, &infoBegin);

	VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	VkClearColorValue color = {0.8984375f, 0.8984375f, 0.9765625f, 1.0f};
	vkCmdClearColorImage(cmdbuf, img,
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

	vkEndCommandBuffer(cmdbuf);
	return cmdbuf;
}

int drm_init() {
	int fd = open("/dev/dri/card0", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	if (!drmIsMaster(fd)) {
		fprintf(stderr, "not drm master\n");
		return -1;
	}
	if(drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "no atomic modesetting\n");
		return -1;
	}
	return fd;
}

int scanout(int fd, uint32_t handle) { //TODO add arguments
	uint32_t handles[4] = {0};
	uint32_t strides[4] = {0};
	uint32_t offsets[4] = {0};
	uint64_t modifiers[4] = {0};

	uint32_t width = 1366;
	uint32_t height = 768;
	uint32_t format = DRM_FORMAT_XRGB8888;
	handles[0] = handle;
	strides[0] = 5464;
	offsets[0] = 0;
	modifiers[0] = 0;

	drmModeAtomicReq *req = drmModeAtomicAlloc();
	uint32_t fb_id;
	if (drmModeAddFB2WithModifiers(fd, width, height, format,
		 handles, strides, offsets, modifiers, &fb_id, 0)) {
		perror("drmModeAddFB2WithModifiers");
		return -1;
	}
	if (drmModeAtomicAddProperty(req, 28, 16, fb_id) < 0) {
		perror("drmModeAtomicAddProperty");
		return -1;
	}
	if (drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_NONBLOCK, 0)) {
		perror("drmModeAtomicCommit");
		return -1;
	}
	drmModeAtomicFree(req);
}

int restore(int fd) {
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (drmModeAtomicAddProperty(req, 28, 16, 88) < 0) {
		perror("drmModeAtomicAddProperty");
		return -1;
	}
	if (drmModeAtomicCommit(fd, req, 0, 0)) {
		perror("drmModeAtomicCommit");
		return -1;
	}
	drmModeAtomicFree(req);
}

void drm_fini(int fd) {
	close(fd);
}

drm_intel_bufmgr *intel_init(int fd) {
	drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 32);
	if (!bufmgr)
		fprintf(stderr, "drm_intel_bufmgr_gem_init failed\n");
	return bufmgr;
}

drm_intel_bo *intel_import(drm_intel_bufmgr *bufmgr, int prime_fd) {
	drm_intel_bo *intel_bo =
	 drm_intel_bo_gem_create_from_prime(bufmgr, prime_fd, 0); // let it query the size
	if (!intel_bo)
		fprintf(stderr, "drm_intel_bo_gem_create_from_prime failed\n");
	uint32_t tiling_mode, swizzle_mode;
	if (drm_intel_bo_get_tiling(intel_bo, &tiling_mode, &swizzle_mode)) {
		fprintf(stderr, "failed to get tiling info\n");
	}
	printf("tiling_mode is %zu\n", tiling_mode);
	return intel_bo;
}

void intel_fini(drm_intel_bufmgr *bufmgr, drm_intel_bo *intel_bo) {
	drm_intel_bo_unreference(intel_bo);
	drm_intel_bufmgr_destroy(bufmgr);
}

int main() {
	VkInstance instance = create_instance();
	if (instance == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkPhysicalDevice physical_device = get_physical_device(instance);
	if (physical_device == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkDevice device = create_device(instance, physical_device);
	if (device == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkImage image = create_image(device, 1366, 768);
	VkDeviceMemory memory = allocate_memory(physical_device, device, image);
	if (vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
		fprintf(stderr, "vkBindImageMemory failed\n");
		return EXIT_FAILURE;
	}

	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);

	VkCommandPool command_pool = create_command_pool(device);
	if (command_pool == VK_NULL_HANDLE)
		return EXIT_FAILURE;

	VkCommandBuffer cmd_clear = record_command_clear(device, command_pool, image);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_clear
	};
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	VkImageSubresource subresource = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel = 0,
		.arrayLayer = 0
	};
	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(device, image, &subresource, &layout);
	printf("Created image with:\noffset %d\nsize %d\nrowPitch %d\narrayPitch %d\ndepthPitch %d\n",
	 layout.offset, layout.size, layout.rowPitch, layout.arrayPitch,
	  layout.depthPitch);

	VkMemoryGetFdInfoKHR getFdInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
		.pNext = NULL,
		.memory = memory,
		.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
	};
	int fd;
	vkGetMemoryFd(device, &getFdInfo, &fd);
	printf("Exported fd %d\n", fd);

/* drm code */
	int drm_fd = drm_init();
	if (drm_fd < 0)
		return EXIT_FAILURE;

	drm_intel_bufmgr *bufmgr = intel_init(drm_fd);
	drm_intel_bo *intel_bo = intel_import(bufmgr, fd);
	if (!intel_bo)
		return EXIT_FAILURE;

	if (scanout(drm_fd, intel_bo->handle) < 0)
		return EXIT_FAILURE;
	sleep(1);
	if (restore(drm_fd) < 0)
		return EXIT_FAILURE;
	intel_fini(bufmgr, intel_bo);
	drm_fini(drm_fd);

	vkFreeCommandBuffers(device, command_pool, 1, &cmd_clear);

	vkFreeMemory(device, memory, NULL);
	vkDestroyImage(device, image, NULL);
	vkDestroyCommandPool(device, command_pool, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroyInstance(instance, NULL);

	return EXIT_SUCCESS;
}
