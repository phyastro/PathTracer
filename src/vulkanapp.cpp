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

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

#define ISDEBUG

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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

    void InitWindow() {
        SDL_Init(SDL_INIT_VIDEO);

        Uint32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Vulkan App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, WindowFlags);
    }

	bool checkValidationLayerSupport() {
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

	void CreateInstance() {
		if (isValidationLayersEnabled && !checkValidationLayerSupport()) {
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

		uint32_t SDLExtensionsCount = 0;
		const char** SDLExtensionsNames;
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, nullptr);
		SDLExtensionsNames = new const char*[SDLExtensionsCount];
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, SDLExtensionsNames);
		std::cout << "Required Extensions:" << std::endl;
		for(uint32_t i = 0; i < SDLExtensionsCount; i++) {
			std::cout << "\t" << SDLExtensionsNames[i] << std::endl;
		}
		createInfo.enabledExtensionCount = SDLExtensionsCount;
		createInfo.ppEnabledExtensionNames = SDLExtensionsNames;
		delete[] SDLExtensionsNames;

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

    void InitVulkan() {
        CreateInstance();
    }
    
    void MainLoop() {
        bool isRunning = true;

        while (isRunning) {
            ManageSDL::SDLHandleEvents(isRunning);
        }
    }

    void CleanUp() {
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