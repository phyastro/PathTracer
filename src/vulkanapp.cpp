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
#include <fstream>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
const bool VSYNC = false;
const int MAX_FRAMES_IN_FLIGHT = 2;

#define ISDEBUG

#ifdef ISDEBUG
const bool isValidationLayersEnabled = true;
#else
const bool isValidationLayersEnabled = false;
#endif

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

struct colorFormatName {
	VkFormat format;
	const char* name;
};

struct colorSpaceName {
	VkColorSpaceKHR space;
	const char* name;
};

struct presentModeName {
	VkPresentModeKHR mode;
	const char* name;
};

struct Vertex {
	glm::vec2 pos;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0; // Only 1 Vertex Array, So Index Is 0
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static VkVertexInputAttributeDescription getAttributeDescription() {
		VkVertexInputAttributeDescription attributeDescription{};
		attributeDescription.binding = 0;
		attributeDescription.location = 0; // location Directive In Vertex Shader
		attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription.offset = offsetof(Vertex, pos);

		return attributeDescription;
	}
};

struct UniformBufferObject {
	alignas(16) glm::vec4 dummy[25];
};

struct PushConstantValues {
	glm::vec2 resolution;
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> instanceExtensions = {
	VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<colorFormatName> colorFormats = {
	{VK_FORMAT_R4G4_UNORM_PACK8, "VK_FORMAT_R4G4_UNORM_PACK8"}, 
	{VK_FORMAT_R4G4B4A4_UNORM_PACK16, "VK_FORMAT_R4G4B4A4_UNORM_PACK16"}, 
	{VK_FORMAT_B4G4R4A4_UNORM_PACK16, "VK_FORMAT_B4G4R4A4_UNORM_PACK16"}, 
	{VK_FORMAT_R5G6B5_UNORM_PACK16, "VK_FORMAT_R5G6B5_UNORM_PACK16"}, 
	{VK_FORMAT_B5G6R5_UNORM_PACK16, "VK_FORMAT_B5G6R5_UNORM_PACK16"}, 
	{VK_FORMAT_R5G5B5A1_UNORM_PACK16, "VK_FORMAT_R5G5B5A1_UNORM_PACK16"}, 
	{VK_FORMAT_B5G5R5A1_UNORM_PACK16, "VK_FORMAT_B5G5R5A1_UNORM_PACK16"}, 
	{VK_FORMAT_A1R5G5B5_UNORM_PACK16, "VK_FORMAT_A1R5G5B5_UNORM_PACK16"}, 
	{VK_FORMAT_R8_UNORM, "VK_FORMAT_R8_UNORM"}, 
	{VK_FORMAT_R8_SNORM, "VK_FORMAT_R8_SNORM"}, 
	{VK_FORMAT_R8_USCALED, "VK_FORMAT_R8_USCALED"}, 
	{VK_FORMAT_R8_SSCALED, "VK_FORMAT_R8_SSCALED"}, 
	{VK_FORMAT_R8_UINT, "VK_FORMAT_R8_UINT"}, 
	{VK_FORMAT_R8_SINT, "VK_FORMAT_R8_SINT"}, 
	{VK_FORMAT_R8_SRGB, "VK_FORMAT_R8_SRGB"}, 
	{VK_FORMAT_R8G8_UNORM, "VK_FORMAT_R8G8_UNORM"}, 
	{VK_FORMAT_R8G8_SNORM, "VK_FORMAT_R8G8_SNORM"}, 
	{VK_FORMAT_R8G8_USCALED, "VK_FORMAT_R8G8_USCALED"}, 
	{VK_FORMAT_R8G8_SSCALED, "VK_FORMAT_R8G8_SSCALED"}, 
	{VK_FORMAT_R8G8_UINT, "VK_FORMAT_R8G8_UINT"}, 
	{VK_FORMAT_R8G8_SINT, "VK_FORMAT_R8G8_SINT"}, 
	{VK_FORMAT_R8G8_SRGB, "VK_FORMAT_R8G8_SRGB"}, 
	{VK_FORMAT_R8G8B8_UNORM, "VK_FORMAT_R8G8B8_UNORM"}, 
	{VK_FORMAT_R8G8B8_SNORM, "VK_FORMAT_R8G8B8_SNORM"}, 
	{VK_FORMAT_R8G8B8_USCALED, "VK_FORMAT_R8G8B8_USCALED"}, 
	{VK_FORMAT_R8G8B8_SSCALED, "VK_FORMAT_R8G8B8_SSCALED"}, 
	{VK_FORMAT_R8G8B8_UINT, "VK_FORMAT_R8G8B8_UINT"}, 
	{VK_FORMAT_R8G8B8_SINT, "VK_FORMAT_R8G8B8_SINT"}, 
	{VK_FORMAT_R8G8B8_SRGB, "VK_FORMAT_R8G8B8_SRGB"}, 
	{VK_FORMAT_B8G8R8_UNORM, "VK_FORMAT_B8G8R8_UNORM"}, 
	{VK_FORMAT_B8G8R8_SNORM, "VK_FORMAT_B8G8R8_SNORM"}, 
	{VK_FORMAT_B8G8R8_USCALED, "VK_FORMAT_B8G8R8_USCALED"}, 
	{VK_FORMAT_B8G8R8_SSCALED, "VK_FORMAT_B8G8R8_SSCALED"}, 
	{VK_FORMAT_B8G8R8_UINT, "VK_FORMAT_B8G8R8_UINT"}, 
	{VK_FORMAT_B8G8R8_SINT, "VK_FORMAT_B8G8R8_SINT"}, 
	{VK_FORMAT_B8G8R8_SRGB, "VK_FORMAT_B8G8R8_SRGB"}, 
	{VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM"}, 
	{VK_FORMAT_R8G8B8A8_SNORM, "VK_FORMAT_R8G8B8A8_SNORM"}, 
	{VK_FORMAT_R8G8B8A8_USCALED, "VK_FORMAT_R8G8B8A8_USCALED"}, 
	{VK_FORMAT_R8G8B8A8_SSCALED, "VK_FORMAT_R8G8B8A8_SSCALED"}, 
	{VK_FORMAT_R8G8B8A8_UINT, "VK_FORMAT_R8G8B8A8_UINT"}, 
	{VK_FORMAT_R8G8B8A8_SINT, "VK_FORMAT_R8G8B8A8_SINT"}, 
	{VK_FORMAT_R8G8B8A8_SRGB, "VK_FORMAT_R8G8B8A8_SRGB"}, 
	{VK_FORMAT_B8G8R8A8_UNORM, "VK_FORMAT_B8G8R8A8_UNORM"}, 
	{VK_FORMAT_B8G8R8A8_SNORM, "VK_FORMAT_B8G8R8A8_SNORM"}, 
	{VK_FORMAT_B8G8R8A8_USCALED, "VK_FORMAT_B8G8R8A8_USCALED"}, 
	{VK_FORMAT_B8G8R8A8_SSCALED, "VK_FORMAT_B8G8R8A8_SSCALED"}, 
	{VK_FORMAT_B8G8R8A8_UINT, "VK_FORMAT_B8G8R8A8_UINT"}, 
	{VK_FORMAT_B8G8R8A8_SINT, "VK_FORMAT_B8G8R8A8_SINT"}, 
	{VK_FORMAT_B8G8R8A8_SRGB, "VK_FORMAT_B8G8R8A8_SRGB"}, 
	{VK_FORMAT_A8B8G8R8_UNORM_PACK32, "VK_FORMAT_A8B8G8R8_UNORM_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_SNORM_PACK32, "VK_FORMAT_A8B8G8R8_SNORM_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_USCALED_PACK32, "VK_FORMAT_A8B8G8R8_USCALED_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_SSCALED_PACK32, "VK_FORMAT_A8B8G8R8_SSCALED_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_UINT_PACK32, "VK_FORMAT_A8B8G8R8_UINT_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_SINT_PACK32, "VK_FORMAT_A8B8G8R8_SINT_PACK32"}, 
	{VK_FORMAT_A8B8G8R8_SRGB_PACK32, "VK_FORMAT_A8B8G8R8_SRGB_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_UNORM_PACK32, "VK_FORMAT_A2R10G10B10_UNORM_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_SNORM_PACK32, "VK_FORMAT_A2R10G10B10_SNORM_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_USCALED_PACK32, "VK_FORMAT_A2R10G10B10_USCALED_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_SSCALED_PACK32, "VK_FORMAT_A2R10G10B10_SSCALED_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_UINT_PACK32, "VK_FORMAT_A2R10G10B10_UINT_PACK32"}, 
	{VK_FORMAT_A2R10G10B10_SINT_PACK32, "VK_FORMAT_A2R10G10B10_SINT_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_UNORM_PACK32, "VK_FORMAT_A2B10G10R10_UNORM_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_SNORM_PACK32, "VK_FORMAT_A2B10G10R10_SNORM_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_USCALED_PACK32, "VK_FORMAT_A2B10G10R10_USCALED_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_SSCALED_PACK32, "VK_FORMAT_A2B10G10R10_SSCALED_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_UINT_PACK32, "VK_FORMAT_A2B10G10R10_UINT_PACK32"}, 
	{VK_FORMAT_A2B10G10R10_SINT_PACK32, "VK_FORMAT_A2B10G10R10_SINT_PACK32"}, 
	{VK_FORMAT_R16_UNORM, "VK_FORMAT_R16_UNORM"}, 
	{VK_FORMAT_R16_SNORM, "VK_FORMAT_R16_SNORM"}, 
	{VK_FORMAT_R16_USCALED, "VK_FORMAT_R16_USCALED"}, 
	{VK_FORMAT_R16_SSCALED, "VK_FORMAT_R16_SSCALED"}, 
	{VK_FORMAT_R16_UINT, "VK_FORMAT_R16_UINT"}, 
	{VK_FORMAT_R16_SINT, "VK_FORMAT_R16_SINT"}, 
	{VK_FORMAT_R16_SFLOAT, "VK_FORMAT_R16_SFLOAT"}, 
	{VK_FORMAT_R16G16_UNORM, "VK_FORMAT_R16G16_UNORM"}, 
	{VK_FORMAT_R16G16_SNORM, "VK_FORMAT_R16G16_SNORM"}, 
	{VK_FORMAT_R16G16_USCALED, "VK_FORMAT_R16G16_USCALED"}, 
	{VK_FORMAT_R16G16_SSCALED, "VK_FORMAT_R16G16_SSCALED"}, 
	{VK_FORMAT_R16G16_UINT, "VK_FORMAT_R16G16_UINT"}, 
	{VK_FORMAT_R16G16_SINT, "VK_FORMAT_R16G16_SINT"}, 
	{VK_FORMAT_R16G16_SFLOAT, "VK_FORMAT_R16G16_SFLOAT"}, 
	{VK_FORMAT_R16G16B16_UNORM, "VK_FORMAT_R16G16B16_UNORM"}, 
	{VK_FORMAT_R16G16B16_SNORM, "VK_FORMAT_R16G16B16_SNORM"}, 
	{VK_FORMAT_R16G16B16_USCALED, "VK_FORMAT_R16G16B16_USCALED"}, 
	{VK_FORMAT_R16G16B16_SSCALED, "VK_FORMAT_R16G16B16_SSCALED"}, 
	{VK_FORMAT_R16G16B16_UINT, "VK_FORMAT_R16G16B16_UINT"}, 
	{VK_FORMAT_R16G16B16_SINT, "VK_FORMAT_R16G16B16_SINT"}, 
	{VK_FORMAT_R16G16B16_SFLOAT, "VK_FORMAT_R16G16B16_SFLOAT"}, 
	{VK_FORMAT_R16G16B16A16_UNORM, "VK_FORMAT_R16G16B16A16_UNORM"}, 
	{VK_FORMAT_R16G16B16A16_SNORM, "VK_FORMAT_R16G16B16A16_SNORM"}, 
	{VK_FORMAT_R16G16B16A16_USCALED, "VK_FORMAT_R16G16B16A16_USCALED"}, 
	{VK_FORMAT_R16G16B16A16_SSCALED, "VK_FORMAT_R16G16B16A16_SSCALED"}, 
	{VK_FORMAT_R16G16B16A16_UINT, "VK_FORMAT_R16G16B16A16_UINT"}, 
	{VK_FORMAT_R16G16B16A16_SINT, "VK_FORMAT_R16G16B16A16_SINT"}, 
	{VK_FORMAT_R16G16B16A16_SFLOAT, "VK_FORMAT_R16G16B16A16_SFLOAT"}, 
	{VK_FORMAT_R32_UINT, "VK_FORMAT_R32_UINT"}, 
	{VK_FORMAT_R32_SINT, "VK_FORMAT_R32_SINT"}, 
	{VK_FORMAT_R32_SFLOAT, "VK_FORMAT_R32_SFLOAT"}, 
	{VK_FORMAT_R32G32_UINT, "VK_FORMAT_R32G32_UINT"}, 
	{VK_FORMAT_R32G32_SINT, "VK_FORMAT_R32G32_SINT"}, 
	{VK_FORMAT_R32G32_SFLOAT, "VK_FORMAT_R32G32_SFLOAT"}, 
	{VK_FORMAT_R32G32B32_UINT, "VK_FORMAT_R32G32B32_UINT"}, 
	{VK_FORMAT_R32G32B32_SINT, "VK_FORMAT_R32G32B32_SINT"}, 
	{VK_FORMAT_R32G32B32_SFLOAT, "VK_FORMAT_R32G32B32_SFLOAT"}, 
	{VK_FORMAT_R32G32B32A32_UINT, "VK_FORMAT_R32G32B32A32_UINT"}, 
	{VK_FORMAT_R32G32B32A32_SINT, "VK_FORMAT_R32G32B32A32_SINT"}, 
	{VK_FORMAT_R32G32B32A32_SFLOAT, "VK_FORMAT_R32G32B32A32_SFLOAT"}, 
	{VK_FORMAT_R64_UINT, "VK_FORMAT_R64_UINT"}, 
	{VK_FORMAT_R64_SINT, "VK_FORMAT_R64_SINT"}, 
	{VK_FORMAT_R64_SFLOAT, "VK_FORMAT_R64_SFLOAT"}, 
	{VK_FORMAT_R64G64_UINT, "VK_FORMAT_R64G64_UINT"}, 
	{VK_FORMAT_R64G64_SINT, "VK_FORMAT_R64G64_SINT"}, 
	{VK_FORMAT_R64G64_SFLOAT, "VK_FORMAT_R64G64_SFLOAT"}, 
	{VK_FORMAT_R64G64B64_UINT, "VK_FORMAT_R64G64B64_UINT"}, 
	{VK_FORMAT_R64G64B64_SINT, "VK_FORMAT_R64G64B64_SINT"}, 
	{VK_FORMAT_R64G64B64_SFLOAT, "VK_FORMAT_R64G64B64_SFLOAT"}, 
	{VK_FORMAT_R64G64B64A64_UINT, "VK_FORMAT_R64G64B64A64_UINT"}, 
	{VK_FORMAT_R64G64B64A64_SINT, "VK_FORMAT_R64G64B64A64_SINT"}, 
	{VK_FORMAT_R64G64B64A64_SFLOAT, "VK_FORMAT_R64G64B64A64_SFLOAT"}, 
	{VK_FORMAT_B10G11R11_UFLOAT_PACK32, "VK_FORMAT_B10G11R11_UFLOAT_PACK32"}
};

const std::vector<colorSpaceName> colorSpaces = {
	{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR"}, 
	{VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT, "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT"}, 
	{VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT, "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT"}, 
	{VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT, "VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT"}, 
	{VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT, "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT"}, 
	{VK_COLOR_SPACE_BT709_LINEAR_EXT, "VK_COLOR_SPACE_BT709_LINEAR_EXT"}, 
	{VK_COLOR_SPACE_BT709_NONLINEAR_EXT, "VK_COLOR_SPACE_BT709_NONLINEAR_EXT"}, 
	{VK_COLOR_SPACE_BT2020_LINEAR_EXT, "VK_COLOR_SPACE_BT2020_LINEAR_EXT"}, 
	{VK_COLOR_SPACE_HDR10_ST2084_EXT, "VK_COLOR_SPACE_HDR10_ST2084_EXT"}, 
	{VK_COLOR_SPACE_DOLBYVISION_EXT, "VK_COLOR_SPACE_DOLBYVISION_EXT"}, 
	{VK_COLOR_SPACE_HDR10_HLG_EXT, "VK_COLOR_SPACE_HDR10_HLG_EXT"}, 
	{VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT, "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT"}, 
	{VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT, "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT"}, 
	{VK_COLOR_SPACE_PASS_THROUGH_EXT, "VK_COLOR_SPACE_PASS_THROUGH_EXT"}, 
	{VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT, "VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT"}, 
	{VK_COLOR_SPACE_DISPLAY_NATIVE_AMD, "VK_COLOR_SPACE_DISPLAY_NATIVE_AMD"}, 
	{VK_COLOR_SPACE_MAX_ENUM_KHR, "VK_COLOR_SPACE_MAX_ENUM_KHR"}
};

const std::vector<presentModeName> presentModes = {
	{VK_PRESENT_MODE_IMMEDIATE_KHR, "VK_PRESENT_MODE_IMMEDIATE_KHR"}, 
	{VK_PRESENT_MODE_MAILBOX_KHR, "VK_PRESENT_MODE_MAILBOX_KHR"}, 
	{VK_PRESENT_MODE_FIFO_KHR, "VK_PRESENT_MODE_FIFO_KHR"}, 
	{VK_PRESENT_MODE_FIFO_RELAXED_KHR, "VK_PRESENT_MODE_FIFO_RELAXED_KHR"}, 
	{VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR"}, 
	{VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR, "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR"}, 
	{VK_PRESENT_MODE_MAX_ENUM_KHR, "VK_PRESENT_MODE_MAX_ENUM_KHR"}
};

const std::vector<Vertex> vertices = {
	{{-1.0f, -1.0f}}, 
	{{1.0f, -1.0f}}, 
	{{1.0f, 1.0f}}, 
	{{-1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

std::vector<char> ReadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed To Open The File!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
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

namespace ManageSDL{
    void SDLHandleEvents(bool& isRunning, bool& isFrameBufferResized) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
			if ((event.window.event == SDL_WINDOWEVENT_RESIZED) && (!isFrameBufferResized)) {
				isFrameBufferResized = true;
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
	int W = WIDTH;
	int H = HEIGHT;

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
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	std::vector<UniformBufferObject> dummyUniform;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<PushConstantValues> pushConstants;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	bool isFrameBufferResized = false;

	uint32_t currentFrame = 0;

    void InitWindow() {
        SDL_Init(SDL_INIT_VIDEO);

        Uint32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Vulkan App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, WindowFlags);
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

	void AvailableInstanceExtensions() {
		uint32_t availableExtensionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());

		std::cout << "Available Instance Extensions:" << std::endl;
		for(const VkExtensionProperties &extension : availableExtensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}
	}

	std::vector<const char*> GetRequiredInstanceExtensions() {
		uint32_t SDLExtensionsCount = 0;
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, nullptr);
		std::vector<const char*> extensions(SDLExtensionsCount);
		SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionsCount, extensions.data());

		if (isValidationLayersEnabled) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Require Extension VK_EXT_debug_utils
		}
		for (const char* instanceExtension : instanceExtensions) {
			extensions.push_back(instanceExtension);
		}

		std::cout << "Required Instance Extensions:" << std::endl;
		for(const char* extension : extensions) {
			std::cout << "\t" << extension << std::endl;
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

		AvailableInstanceExtensions();

		std::vector<const char*> extensions = GetRequiredInstanceExtensions();
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

		std::cout << "Available Device Extensions:" << std::endl;
		for (const VkExtensionProperties& availableExtension : availableExtensions) {
			std::cout << "\t" << availableExtension.extensionName << std::endl;
		}

		std::cout << "Required Device Extensions:" << std::endl;
		for (const char* deviceExtension : deviceExtensions) {
			std::cout << "\t" << deviceExtension << std::endl;
		}

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
		std::cout << "Available Surface Formats:" << std::endl;
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			for (const colorSpaceName& colorSpace : colorSpaces) {
				if (availableFormat.colorSpace == colorSpace.space) {
					for (const colorFormatName& colorFormat : colorFormats) {
						if (availableFormat.format == colorFormat.format) {
							std::cout << "\t" << colorFormat.name << ", " << colorSpace.name << std::endl;
						}
					}
				}
			}
		}

		VkSurfaceFormatKHR surfaceFormat;
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT) {
				surfaceFormat = availableFormat;
				break;
			}
		}

		for (const colorSpaceName& colorSpace : colorSpaces) {
			if (surfaceFormat.colorSpace == colorSpace.space) {
				std::cout << "Using VK_FORMAT_R16G16B16A16_SFLOAT Format With " << colorSpace.name << " Color Space" << std::endl;
				return surfaceFormat;
			}
		}

		throw std::runtime_error("Failed To Choose Appropriate Surface Format!");
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		std::vector<presentModeName> availablePresentModesNames;
		std::cout << "Available Presentation Modes:" << std::endl;
		for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
			for (const presentModeName& presentMode : presentModes) {
				if (availablePresentMode == presentMode.mode) {
					std::cout << "\t" << presentMode.name << std::endl;
					availablePresentModesNames.push_back(presentMode);
					break;
				}
			}
		}

		if (VSYNC) {
			for (const presentModeName& availablePresentModeName : availablePresentModesNames) {
				if (availablePresentModeName.mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					std::cout << "Using Presentation Mode VK_PRESENT_MODE_MAILBOX_KHR" << std::endl;
					return availablePresentModeName.mode;
				}
			}

			std::cout << "Using Presentation Mode VK_PRESENT_MODE_FIFO_KHR" << std::endl;
			return VK_PRESENT_MODE_FIFO_KHR;
		} else {
			for (const presentModeName& availablePresentModeName : availablePresentModesNames) {
				if (availablePresentModeName.mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					std::cout << "Using Presentation Mode VK_PRESENT_MODE_IMMEDIATE_KHR" << std::endl;
					return availablePresentModeName.mode;
				}
			}

			throw std::runtime_error("Disabling VSync Is Not Supported!");
		}
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			W = capabilities.currentExtent.width;
			H = capabilities.currentExtent.height;
			return capabilities.currentExtent;
		} else {
			SDL_GetWindowSizeInPixels(window, &W, &H);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(W), 
				static_cast<uint32_t>(H)
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

	void CreateRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear The Framebuffer Before Drawing New Frame
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		//subpass.pInputAttachments =  // Attachments That Are Read From A Shader
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // Index 0, The Only Subpass Which We Use
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Render Pass!");
		}
	}

	void CreateDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // For Image Sampling Related Descriptors
		
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Descriptor Set Layout!");
		}
	}

	VkShaderModule CreateShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Shader Module!");
		}

		return shaderModule;
	}

	void CreateGraphicsPipeline() {
		auto vertexShaderCode = ReadFile("vertex.spv");
		auto fragmentShaderCode = ReadFile("fragment.spv");

		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShaderModule;
		vertexShaderStageInfo.pName = "main";
		vertexShaderStageInfo.pSpecializationInfo = nullptr; // For Specifying Different Constants Instead Of Doing It In Runtime

		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShaderModule;
		fragmentShaderStageInfo.pName = "main";
		fragmentShaderStageInfo.pSpecializationInfo = nullptr; // For Specifying Different Constants Instead Of Doing It In Runtime

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		VkVertexInputAttributeDescription attributeDescription = Vertex::getAttributeDescription();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(PushConstantValues);
		pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstants.resize(MAX_FRAMES_IN_FLIGHT);

		// Uniforms And Push Values
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Pipeline Layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Graphics Pipeline!");
		}

		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	}

	void CreateFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Create Framebuffer!");
			}
		}
	}

	void CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Command Pool!");
		}
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
				return i;
			}
		}

		throw std::runtime_error("Failed To Find Suitable Memory Type!");
	}

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Buffer!");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Buffer Memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = commandPool;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		vertexBuffer, vertexBufferMemory);

		CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void CreateIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		indexBuffer, indexBufferMemory);

		CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void CreateUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		dummyUniform.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void CreateDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Descriptor Pool!");
		}
	}

	void CreateDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocateInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Descriptor Sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	void CreateCommandBuffers() {
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Command Buffers!");
		}
	}

	void CreateSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if ((vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) || 
			(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) || 
			(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)) {
				throw std::runtime_error("Failed To Create Sync Objects For A Frame!");
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
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
    }

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Begin Recording Command Buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		pushConstants[currentFrame].resolution = glm::vec2(W, H);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants[currentFrame]), &pushConstants[currentFrame]);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Record Command Buffer!");
		}
	}

	void CleanUpSwapChain() {
		for (VkFramebuffer framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		for (VkImageView imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void RecreateSwapChain() {
		SDL_Event event;
		while ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0) {
			SDL_WaitEvent(&event);
		}

		vkDeviceWaitIdle(device);

		CleanUpSwapChain();
		
		CreateSwapChain();
		CreateImageViews();
		CreateFramebuffers();
	}

	void UpdateUniformBuffer(uint32_t currentImage) {
		for (int i = 0; i < 25; i++) {
			dummyUniform[currentFrame].dummy[i] = glm::vec4(glm::ivec4(4*i+1, 4*i+2, 4*i+3, 4*i+4)) * 0.01f;
		}

		memcpy(uniformBuffersMapped[currentImage], &dummyUniform[currentFrame], sizeof(dummyUniform[currentFrame]));
	}

	void DrawFrame() {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
		imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		} else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
			throw std::runtime_error("Failed To Acquire Swap Chain Image!");
		}

		UpdateUniformBuffer(currentFrame);

		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Submit Draw Command Buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Checking If Presentation Was Successful For Multiple Swap Chains

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || isFrameBufferResized) {
			isFrameBufferResized = false;
			RecreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed To Present Swap Chain Image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
    
    void MainLoop() {
        bool isRunning = true;

        while (isRunning) {
            ManageSDL::SDLHandleEvents(isRunning, isFrameBufferResized);
			DrawFrame();
        }

		vkDeviceWaitIdle(device);
    }

    void CleanUp() {
		CleanUpSwapChain();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyFence(device, inFlightFences[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		vkDestroyRenderPass(device, renderPass, nullptr);

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