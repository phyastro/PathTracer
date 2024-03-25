#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_image.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

#define ISDEBUG

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef ISDEBUG
	const bool isValidationLayersEnabled = true;
#else
	const bool isValidationLayersEnabled = false;
#endif

namespace ManageSDL{
    void SDLHandleEvents(bool& isRunning) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
const VkAllocationCallbacks *pAllocator, 
VkDebugUtilsMessengerEXT *pDebugMessenger) {
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
	vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestoryDebugUtilsMessengerEXT(VkInstance instance, 
VkDebugUtilsMessengerEXT debugMessenger, 
const VkAllocationCallbacks* pAllocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
	vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanApp {
public:
    void run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        CleanUp();
    }
private:
    SDL_Window* window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

    void InitWindow() {
        SDL_Init(SDL_INIT_VIDEO);

        Uint32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Vulkan App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, WindowFlags);
    }

	bool CheckValidationLayerSupport() {
		uint32_t layersCount;
		vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layersCount);
		vkEnumerateInstanceLayerProperties(&layersCount, availableLayers.data());

		for(const char* layerName : validationLayers) {
			bool layerFound = false;
			for(const VkLayerProperties &layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	std::vector<const char*> GetRequiredExtensions() {
		uint32_t SDLExtensionsCount = 0;
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, nullptr);
		std::vector<const char*> extensions(SDLExtensionsCount);
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, extensions.data());
		if (isValidationLayersEnabled) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		std::cout << "Required Extensions:" << std::endl;
		for(const char* extensionName : extensions) {
			std::cout << "\t" << extensionName << std::endl;
		}

		return extensions;
	}

	void CreateInstance() {
		if (isValidationLayersEnabled && !CheckValidationLayerSupport()) {
			throw std::runtime_error("Validation Layers Are Requested But Not Available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan App";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t availableExtensionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());
		std::cout << "Available Extensions:" << std::endl;
		for(const VkExtensionProperties &extension : availableExtensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}

		std::vector<const char*> extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (isValidationLayersEnabled) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Instance Creation Failed!");
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
	VkDebugUtilsMessageTypeFlagsEXT messageType, 
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, 
	void *pUserData) {
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void SetupDebugMessenger() {
		if (!isValidationLayersEnabled) {
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Set Up Debug Messenger!");
		}
	}

	void CreateSurface() {
		if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
			throw std::runtime_error("Window Surface Creation Failed!");
		}
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}
			if (indices.IsComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const VkExtensionProperties& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool isExtensionsSupported = CheckDeviceExtensionsSupport(device);

		bool isSwapChainAdequate = false;
		if (isExtensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.IsComplete() && isExtensionsSupported && isSwapChainAdequate;
	}

	int RateDeviceSuitability(VkPhysicalDevice device) {
		int score = 0;
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			score += 100;
		}
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
			score += 300;
		}
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		if (!IsDeviceSuitable(device)) {
			return 0;
		}

		return score;
	}

	void PickPhysicalDevice() {
		uint32_t devicesCount;
		vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);
		if (devicesCount == 0) {
			throw std::runtime_error("Failed To Find GPUs With Vulkan Support!");
		}
		std::vector<VkPhysicalDevice> devices(devicesCount);
		vkEnumeratePhysicalDevices(instance, &devicesCount, devices.data());
		std::multimap<int, VkPhysicalDevice> deviceScores;
		for (const VkPhysicalDevice &device : devices) {
			int score = RateDeviceSuitability(device);
			deviceScores.insert(std::make_pair(score, device));
		}

		if (deviceScores.rbegin()->first > 0) {
			physicalDevice = deviceScores.rbegin()->second;
		} else {
			throw std::runtime_error("Failed To Find A Suitable GPU!");
		}
	}

	void CreateLogicalDevice() {
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (isValidationLayersEnabled) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Logical Device Creation Failed!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				std::cout << "Using VK_FORMAT_B8G8R8A8_SRGB With VK_COLOR_SPACE_SRGB_NONLINEAR_KHR" << std::endl;
				return availableFormat;
			}
		}

		std::cout << "Using Available Format With Available Colorspace" << std::endl;
		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "Using Presentation Mode VK_PRESENT_MODE_MAILBOX_KHR" << std::endl;
				return availablePresentMode;
			}
		}

		std::cout << "Using Presentation Mode VK_PRESENT_MODE_FIFO_KHR" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width;
			int height;
			SDL_GetWindowSizeInPixels(window, &width, &height);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width), 
				static_cast<uint32_t>(height)
			};
			actualExtent.width = std::clamp(actualExtent.width, 
				capabilities.minImageExtent.width, 
				capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, 
				capabilities.minImageExtent.height, 
				capabilities.maxImageExtent.height);
			
			return actualExtent;
		}
	}

	void CreateSwapChain() {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if ((swapChainSupport.capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.capabilities.maxImageCount)) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// VK_IMAGE_USAGE_TRANSFER_DST_BIT For Seperate Post-Processing And Then Display

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Slower
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Faster
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // No Transformation To Images
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore Alpha Channels
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_FALSE; // We Need To Care About Pixels Which Are Not Visible So, VK_FALSE

		createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO, We Need This To Be Enabled To Resize Window, Etc.

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Swap Chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void CreateImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
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

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Create Image Views!");
			}
		}
	}

    void InitVulkan() {
        CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
    }
    
    void MainLoop() {
        bool isRunning = true;

        while (isRunning) {
            ManageSDL::SDLHandleEvents(isRunning);
        }
    }

    void CleanUp() {
		for (VkImageView imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		vkDestroyDevice(device, nullptr);

		if (isValidationLayersEnabled) {
			DestoryDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[])
{
    VulkanApp app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}