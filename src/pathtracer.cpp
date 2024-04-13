#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_image.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
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
#include <array>
#include <cmath>

const unsigned int WIDTH = 1280;
const unsigned int HEIGHT = 720;
const int MAX_FRAMES_IN_FLIGHT = 2;
const bool OFFSCREENRENDER = false;
const int NUMSAMPLES = 10000;
const int NUMSAMPLESPERFRAME = 5;
const int PATHLENGTH = 10000;
const int TONEMAP = 3; // 0 - None,  1 - Reinhard, 2 - ACES Film, 3 - DEUCES

#define DEBUGMODE
#define MAX_OBJECTS_SIZE 1024
#define MAX_MATERIALS_SIZE 1024

#ifdef DEBUGMODE
const bool isValidationLayersEnabled = true;
#else
const bool isValidationLayersEnabled = false;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsComputeFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsComputeFamily.has_value() && presentFamily.has_value();
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

struct sphere {
	float pos[3];
	float radius;
	int materialID;
};

struct plane {
	float pos[3];
	int materialID;
};

struct box {
	float pos[3];
	float rotation[3];
	float size[3];
	int materialID;
};

struct lens {
	float pos[3];
	float rotation[3];
	float radius;
	float focalLength;
	float thickness;
	bool isConverging;
	int materialID;
};

struct material {
	float reflection[3];
	float emission[2];
};

struct Camera {
	glm::vec3 pos;
	glm::vec2 angle;
	int ISO;
	float size;
	float apertureSize;
	float apertureDist;
	float lensRadius;
	float lensFocalLength;
	float lensThickness;
	float lensDistance;
};

struct UniformBufferObject {
	float numObjects[4];
	float packedObjects[MAX_OBJECTS_SIZE];
	float packedMaterials[MAX_MATERIALS_SIZE];
	float CIEXYZ2006[1323];
};

struct PushConstantValues {
	glm::ivec2 resolution;
	int frame;
	int samples;
	int samplesPerFrame;
	float FPS;
	float persistence;
	int pathLength;
	glm::vec2 cameraAngle;
	float cameraPosX;
	float cameraPosY;
	float cameraPosZ;
	int ISO;
	float cameraSize;
	float apertureSize;
	float apertureDist;
	float lensRadius;
	float lensFocalLength;
	float lensThickness;
	float lensDistance;
	int tonemap;
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> instanceExtensions = {
	VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
	VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME
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

// Table
// http://www.cvrl.org/ciexyzpr.htm
const float CIEXYZ2006[1323] = {
    0.00295242f, 	0.0004076779f, 	0.01318752f,
    0.003577275f, 	0.0004977769f, 	0.01597879f,
    0.004332146f, 	0.0006064754f, 	0.01935758f,
    0.005241609f, 	0.000737004f, 	0.02343758f,
    0.006333902f, 	0.0008929388f, 	0.02835021f,
    0.007641137f, 	0.001078166f, 	0.03424588f,
    0.009199401f, 	0.001296816f, 	0.04129467f,
    0.01104869f, 	0.001553159f, 	0.04968641f,
    0.01323262f, 	0.001851463f, 	0.05962964f,
    0.01579791f, 	0.002195795f, 	0.07134926f,
    0.01879338f, 	0.002589775f, 	0.08508254f,
    0.02226949f, 	0.003036799f, 	0.1010753f,
    0.02627978f, 	0.003541926f, 	0.1195838f,
    0.03087862f, 	0.004111422f, 	0.1408647f,
    0.0361189f, 	0.004752618f, 	0.1651644f,
    0.04204986f, 	0.005474207f, 	0.1927065f,
    0.04871256f, 	0.006285034f, 	0.2236782f,
    0.05612868f, 	0.007188068f, 	0.2582109f,
    0.06429866f, 	0.008181786f, 	0.2963632f,
    0.07319818f, 	0.009260417f, 	0.3381018f,
    0.08277331f, 	0.01041303f, 	0.3832822f,
    0.09295327f, 	0.01162642f, 	0.4316884f,
    0.1037137f, 	0.01289884f, 	0.483244f,
    0.115052f, 	0.01423442f, 	0.5379345f,
    0.1269771f, 	0.0156408f, 	0.595774f,
    0.1395127f, 	0.01712968f, 	0.6568187f,
    0.1526661f, 	0.01871265f, 	0.7210459f,
    0.1663054f, 	0.02038394f, 	0.7878635f,
    0.1802197f, 	0.02212935f, 	0.8563391f,
    0.1941448f, 	0.02392985f, 	0.9253017f,
    0.2077647f, 	0.02576133f, 	0.9933444f,
    0.2207911f, 	0.02760156f, 	1.059178f,
    0.2332355f, 	0.02945513f, 	1.122832f,
    0.2452462f, 	0.03133884f, 	1.184947f,
    0.2570397f, 	0.03327575f, 	1.246476f,
    0.2688989f, 	0.03529554f, 	1.308674f,
    0.2810677f, 	0.03742705f, 	1.372628f,
    0.2933967f, 	0.03967137f, 	1.437661f,
    0.3055933f, 	0.04201998f, 	1.502449f,
    0.3173165f, 	0.04446166f, 	1.565456f,
    0.3281798f, 	0.04698226f, 	1.62494f,
    0.3378678f, 	0.04956742f, 	1.679488f,
    0.3465097f, 	0.05221219f, 	1.729668f,
    0.3543953f, 	0.05491387f, 	1.776755f,
    0.3618655f, 	0.05766919f, 	1.822228f,
    0.3693084f, 	0.06047429f, 	1.867751f,
    0.3770107f, 	0.06332195f, 	1.914504f,
    0.384685f, 	0.06619271f, 	1.961055f,
    0.3918591f, 	0.06906185f, 	2.005136f,
    0.3980192f, 	0.0719019f, 	2.044296f,
    0.4026189f, 	0.07468288f, 	2.075946f,
    0.4052637f, 	0.07738452f, 	2.098231f,
    0.4062482f, 	0.08003601f, 	2.112591f,
    0.406066f, 	0.08268524f, 	2.121427f,
    0.4052283f, 	0.08538745f, 	2.127239f,
    0.4042529f, 	0.08820537f, 	2.132574f,
    0.4034808f, 	0.09118925f, 	2.139093f,
    0.4025362f, 	0.09431041f, 	2.144815f,
    0.4008675f, 	0.09751346f, 	2.146832f,
    0.3979327f, 	0.1007349f, 	2.14225f,
    0.3932139f, 	0.103903f, 	2.128264f,
    0.3864108f, 	0.1069639f, 	2.103205f,
    0.3779513f, 	0.1099676f, 	2.069388f,
    0.3684176f, 	0.1129992f, 	2.03003f,
    0.3583473f, 	0.1161541f, 	1.988178f,
    0.3482214f, 	0.1195389f, 	1.946651f,
    0.338383f, 	0.1232503f, 	1.907521f,
    0.3288309f, 	0.1273047f, 	1.870689f,
    0.3194977f, 	0.1316964f, 	1.835578f,
    0.3103345f, 	0.1364178f, 	1.801657f,
    0.3013112f, 	0.1414586f, 	1.76844f,
    0.2923754f, 	0.1468003f, 	1.735338f,
    0.2833273f, 	0.1524002f, 	1.701254f,
    0.2739463f, 	0.1582021f, 	1.665053f,
    0.2640352f, 	0.16414f, 	1.625712f,
    0.2534221f, 	0.1701373f, 	1.582342f,
    0.2420135f, 	0.1761233f, 	1.534439f,
    0.2299346f, 	0.1820896f, 	1.482544f,
    0.2173617f, 	0.1880463f, 	1.427438f,
    0.2044672f, 	0.1940065f, 	1.369876f,
    0.1914176f, 	0.1999859f, 	1.310576f,
    0.1783672f, 	0.2060054f, 	1.250226f,
    0.1654407f, 	0.2120981f, 	1.189511f,
    0.1527391f, 	0.2183041f, 	1.12905f,
    0.1403439f, 	0.2246686f, 	1.069379f,
    0.1283167f, 	0.2312426f, 	1.010952f,
    0.1167124f, 	0.2380741f, 	0.9541809f,
    0.1056121f, 	0.2451798f, 	0.8995253f,
    0.09508569f, 	0.2525682f, 	0.847372f,
    0.08518206f, 	0.2602479f, 	0.7980093f,
    0.0759312f, 	0.2682271f, 	0.7516389f,
    0.06733159f, 	0.2765005f, 	0.7082645f,
    0.05932018f, 	0.2850035f, 	0.6673867f,
    0.05184106f, 	0.2936475f, 	0.6284798f,
    0.04486119f, 	0.3023319f, 	0.5911174f,
    0.0383677f, 	0.3109438f, 	0.5549619f,
    0.03237296f, 	0.3194105f, 	0.5198843f,
    0.02692095f, 	0.3278683f, 	0.4862772f,
    0.0220407f, 	0.3365263f, 	0.4545497f,
    0.01773951f, 	0.3456176f, 	0.4249955f,
    0.01400745f, 	0.3554018f, 	0.3978114f,
    0.01082291f, 	0.3660893f, 	0.3730218f,
    0.008168996f, 	0.3775857f, 	0.3502618f,
    0.006044623f, 	0.389696f, 	0.3291407f,
    0.004462638f, 	0.4021947f, 	0.3093356f,
    0.00344681f, 	0.4148227f, 	0.2905816f,
    0.003009513f, 	0.4273539f, 	0.2726773f,
    0.003090744f, 	0.4398206f, 	0.2555143f,
    0.003611221f, 	0.452336f, 	0.2390188f,
    0.004491435f, 	0.4650298f, 	0.2231335f,
    0.005652072f, 	0.4780482f, 	0.2078158f,
    0.007035322f, 	0.4915173f, 	0.1930407f,
    0.008669631f, 	0.5054224f, 	0.1788089f,
    0.01060755f, 	0.5197057f, 	0.1651287f,
    0.01290468f, 	0.5343012f, 	0.1520103f,
    0.01561956f, 	0.5491344f, 	0.1394643f,
    0.0188164f, 	0.5641302f, 	0.1275353f,
    0.02256923f, 	0.5792416f, 	0.1163771f,
    0.02694456f, 	0.5944264f, 	0.1061161f,
    0.0319991f, 	0.6096388f, 	0.09682266f,
    0.03778185f, 	0.6248296f, 	0.08852389f,
    0.04430635f, 	0.6399656f, 	0.08118263f,
    0.05146516f, 	0.6550943f, 	0.07463132f,
    0.05912224f, 	0.6702903f, 	0.06870644f,
    0.0671422f, 	0.6856375f, 	0.06327834f,
    0.07538941f, 	0.7012292f, 	0.05824484f,
    0.08376697f, 	0.7171103f, 	0.05353812f,
    0.09233581f, 	0.7330917f, 	0.04914863f,
    0.101194f, 	0.7489041f, 	0.04507511f,
    0.1104362f, 	0.764253f, 	0.04131175f,
    0.1201511f, 	0.7788199f, 	0.03784916f,
    0.130396f, 	0.792341f, 	0.03467234f,
    0.141131f, 	0.804851f, 	0.03175471f,
    0.1522944f, 	0.8164747f, 	0.02907029f,
    0.1638288f, 	0.827352f, 	0.02659651f,
    0.1756832f, 	0.8376358f, 	0.02431375f,
    0.1878114f, 	0.8474653f, 	0.02220677f,
    0.2001621f, 	0.8568868f, 	0.02026852f,
    0.2126822f, 	0.8659242f, 	0.01849246f,
    0.2253199f, 	0.8746041f, 	0.01687084f,
    0.2380254f, 	0.8829552f, 	0.01539505f,
    0.2507787f, 	0.8910274f, 	0.0140545f,
    0.2636778f, 	0.8989495f, 	0.01283354f,
    0.2768607f, 	0.9068753f, 	0.01171754f,
    0.2904792f, 	0.9149652f, 	0.01069415f,
    0.3046991f, 	0.9233858f, 	0.009753f,
    0.3196485f, 	0.9322325f, 	0.008886096f,
    0.3352447f, 	0.9412862f, 	0.008089323f,
    0.351329f, 	0.9502378f, 	0.007359131f,
    0.3677148f, 	0.9587647f, 	0.006691736f,
    0.3841856f, 	0.9665325f, 	0.006083223f,
    0.4005312f, 	0.9732504f, 	0.005529423f,
    0.4166669f, 	0.9788415f, 	0.005025504f,
    0.432542f, 	0.9832867f, 	0.004566879f,
    0.4481063f, 	0.986572f, 	0.004149405f,
    0.4633109f, 	0.9886887f, 	0.003769336f,
    0.478144f, 	0.9897056f, 	0.003423302f,
    0.4927483f, 	0.9899849f, 	0.003108313f,
    0.5073315f, 	0.9899624f, 	0.00282165f,
    0.5221315f, 	0.9900731f, 	0.00256083f,
    0.537417f, 	0.99075f, 	0.002323578f,
    0.5534217f, 	0.9922826f, 	0.002107847f,
    0.5701242f, 	0.9943837f, 	0.001911867f,
    0.5874093f, 	0.9966221f, 	0.001734006f,
    0.6051269f, 	0.9985649f, 	0.001572736f,
    0.6230892f, 	0.9997775f, 	0.001426627f,
    0.6410999f, 	0.999944f, 	0.001294325f,
    0.6590659f, 	0.99922f, 	0.001174475f,
    0.6769436f, 	0.9978793f, 	0.001065842f,
    0.6947143f, 	0.9961934f, 	0.0009673215f,
    0.7123849f, 	0.9944304f, 	0.0008779264f,
    0.7299978f, 	0.9927831f, 	0.0007967847f,
    0.7476478f, 	0.9911578f, 	0.0007231502f,
    0.765425f, 	0.9893925f, 	0.0006563501f,
    0.7834009f, 	0.9873288f, 	0.0005957678f,
    0.8016277f, 	0.9848127f, 	0.0005408385f,
    0.8201041f, 	0.9817253f, 	0.0004910441f,
    0.8386843f, 	0.9780714f, 	0.0004459046f,
    0.8571936f, 	0.973886f, 	0.0004049826f,
    0.8754652f, 	0.9692028f, 	0.0003678818f,
    0.8933408f, 	0.9640545f, 	0.0003342429f,
    0.9106772f, 	0.9584409f, 	0.0003037407f,
    0.9273554f, 	0.9522379f, 	0.0002760809f,
    0.9432502f, 	0.9452968f, 	0.000250997f,
    0.9582244f, 	0.9374773f, 	0.0002282474f,
    0.9721304f, 	0.9286495f, 	0.0002076129f,
    0.9849237f, 	0.9187953f, 	0.0001888948f,
    0.9970067f, 	0.9083014f, 	0.0001719127f,
    1.008907f, 	0.8976352f, 	0.000156503f,
    1.021163f, 	0.8872401f, 	0.0001425177f,
    1.034327f, 	0.877536f, 	0.000129823f,
    1.048753f, 	0.868792f, 	0.0001182974f,
    1.063937f, 	0.8607474f, 	0.000107831f,
    1.079166f, 	0.8530233f, 	0.00009832455f,
    1.093723f, 	0.8452535f, 	0.00008968787f,
    1.106886f, 	0.8370838f, 	0.00008183954f,
    1.118106f, 	0.8282409f, 	0.00007470582f,
    1.127493f, 	0.818732f, 	0.00006821991f,
    1.135317f, 	0.8086352f, 	0.00006232132f,
    1.141838f, 	0.7980296f, 	0.00005695534f,
    1.147304f, 	0.786995f, 	0.00005207245f,
    1.151897f, 	0.775604f, 	0.00004762781f,
    1.155582f, 	0.7638996f, 	0.00004358082f,
    1.158284f, 	0.7519157f, 	0.00003989468f,
    1.159934f, 	0.7396832f, 	0.00003653612f,
    1.160477f, 	0.7272309f, 	0.00003347499f,
    1.15989f, 	0.7145878f, 	0.000030684f,
    1.158259f, 	0.7017926f, 	0.00002813839f,
    1.155692f, 	0.6888866f, 	0.00002581574f,
    1.152293f, 	0.6759103f, 	0.00002369574f,
    1.148163f, 	0.6629035f, 	0.00002175998f,
    1.143345f, 	0.6498911f, 	0.00001999179f,
    1.137685f, 	0.636841f, 	0.00001837603f,
    1.130993f, 	0.6237092f, 	0.00001689896f,
    1.123097f, 	0.6104541f, 	0.00001554815f,
    1.113846f, 	0.5970375f, 	0.00001431231f,
    1.103152f, 	0.5834395f, 	0.00001318119f,
    1.091121f, 	0.5697044f, 	0.00001214548f,
    1.077902f, 	0.5558892f, 	0.00001119673f,
    1.063644f, 	0.5420475f, 	0.00001032727f,
    1.048485f, 	0.5282296f, 	0.00000953013f,
    1.032546f, 	0.5144746f, 	0.000008798979f,
    1.01587f, 	0.5007881f, 	0.000008128065f,
    0.9984859f, 	0.4871687f, 	0.00000751216f,
    0.9804227f, 	0.473616f, 	0.000006946506f,
    0.9617111f, 	0.4601308f, 	0.000006426776f,
    0.9424119f, 	0.446726f, 	0.0f,
    0.9227049f, 	0.4334589f, 	0.0f,
    0.9027804f, 	0.4203919f, 	0.0f,
    0.8828123f, 	0.407581f, 	0.0f,
    0.8629581f, 	0.3950755f, 	0.0f,
    0.8432731f, 	0.3828894f, 	0.0f,
    0.8234742f, 	0.370919f, 	0.0f,
    0.8032342f, 	0.3590447f, 	0.0f,
    0.7822715f, 	0.3471615f, 	0.0f,
    0.7603498f, 	0.3351794f, 	0.0f,
    0.7373739f, 	0.3230562f, 	0.0f,
    0.713647f, 	0.3108859f, 	0.0f,
    0.6895336f, 	0.298784f, 	0.0f,
    0.6653567f, 	0.2868527f, 	0.0f,
    0.6413984f, 	0.2751807f, 	0.0f,
    0.6178723f, 	0.2638343f, 	0.0f,
    0.5948484f, 	0.252833f, 	0.0f,
    0.57236f, 	0.2421835f, 	0.0f,
    0.5504353f, 	0.2318904f, 	0.0f,
    0.5290979f, 	0.2219564f, 	0.0f,
    0.5083728f, 	0.2123826f, 	0.0f,
    0.4883006f, 	0.2031698f, 	0.0f,
    0.4689171f, 	0.1943179f, 	0.0f,
    0.4502486f, 	0.185825f, 	0.0f,
    0.4323126f, 	0.1776882f, 	0.0f,
    0.415079f, 	0.1698926f, 	0.0f,
    0.3983657f, 	0.1623822f, 	0.0f,
    0.3819846f, 	0.1550986f, 	0.0f,
    0.3657821f, 	0.1479918f, 	0.0f,
    0.3496358f, 	0.1410203f, 	0.0f,
    0.3334937f, 	0.1341614f, 	0.0f,
    0.3174776f, 	0.1274401f, 	0.0f,
    0.3017298f, 	0.1208887f, 	0.0f,
    0.2863684f, 	0.1145345f, 	0.0f,
    0.27149f, 	0.1083996f, 	0.0f,
    0.2571632f, 	0.1025007f, 	0.0f,
    0.2434102f, 	0.09684588f, 	0.0f,
    0.2302389f, 	0.09143944f, 	0.0f,
    0.2176527f, 	0.08628318f, 	0.0f,
    0.2056507f, 	0.08137687f, 	0.0f,
    0.1942251f, 	0.07671708f, 	0.0f,
    0.183353f, 	0.07229404f, 	0.0f,
    0.1730097f, 	0.06809696f, 	0.0f,
    0.1631716f, 	0.06411549f, 	0.0f,
    0.1538163f, 	0.06033976f, 	0.0f,
    0.144923f, 	0.05676054f, 	0.0f,
    0.1364729f, 	0.05336992f, 	0.0f,
    0.1284483f, 	0.05016027f, 	0.0f,
    0.120832f, 	0.04712405f, 	0.0f,
    0.1136072f, 	0.04425383f, 	0.0f,
    0.1067579f, 	0.04154205f, 	0.0f,
    0.1002685f, 	0.03898042f, 	0.0f,
    0.09412394f, 	0.03656091f, 	0.0f,
    0.08830929f, 	0.03427597f, 	0.0f,
    0.0828101f, 	0.03211852f, 	0.0f,
    0.07761208f, 	0.03008192f, 	0.0f,
    0.07270064f, 	0.02816001f, 	0.0f,
    0.06806167f, 	0.02634698f, 	0.0f,
    0.06368176f, 	0.02463731f, 	0.0f,
    0.05954815f, 	0.02302574f, 	0.0f,
    0.05564917f, 	0.02150743f, 	0.0f,
    0.05197543f, 	0.02007838f, 	0.0f,
    0.04851788f, 	0.01873474f, 	0.0f,
    0.04526737f, 	0.01747269f, 	0.0f,
    0.04221473f, 	0.01628841f, 	0.0f,
    0.03934954f, 	0.01517767f, 	0.0f,
    0.0366573f, 	0.01413473f, 	0.0f,
    0.03412407f, 	0.01315408f, 	0.0f,
    0.03173768f, 	0.01223092f, 	0.0f,
    0.02948752f, 	0.01136106f, 	0.0f,
    0.02736717f, 	0.0105419f, 	0.0f,
    0.02538113f, 	0.00977505f, 	0.0f,
    0.02353356f, 	0.009061962f, 	0.0f,
    0.02182558f, 	0.008402962f, 	0.0f,
    0.0202559f, 	0.007797457f, 	0.0f,
    0.01881892f, 	0.00724323f, 	0.0f,
    0.0174993f, 	0.006734381f, 	0.0f,
    0.01628167f, 	0.006265001f, 	0.0f,
    0.01515301f, 	0.005830085f, 	0.0f,
    0.0141023f, 	0.005425391f, 	0.0f,
    0.01312106f, 	0.005047634f, 	0.0f,
    0.01220509f, 	0.00469514f, 	0.0f,
    0.01135114f, 	0.004366592f, 	0.0f,
    0.01055593f, 	0.004060685f, 	0.0f,
    0.009816228f, 	0.00377614f, 	0.0f,
    0.009128517f, 	0.003511578f, 	0.0f,
    0.008488116f, 	0.003265211f, 	0.0f,
    0.007890589f, 	0.003035344f, 	0.0f,
    0.007332061f, 	0.002820496f, 	0.0f,
    0.006809147f, 	0.002619372f, 	0.0f,
    0.006319204f, 	0.00243096f, 	0.0f,
    0.005861036f, 	0.002254796f, 	0.0f,
    0.005433624f, 	0.002090489f, 	0.0f,
    0.005035802f, 	0.001937586f, 	0.0f,
    0.004666298f, 	0.001795595f, 	0.0f,
    0.00432375f, 	0.001663989f, 	0.0f,
    0.004006709f, 	0.001542195f, 	0.0f,
    0.003713708f, 	0.001429639f, 	0.0f,
    0.003443294f, 	0.001325752f, 	0.0f,
    0.003194041f, 	0.00122998f, 	0.0f,
    0.002964424f, 	0.001141734f, 	0.0f,
    0.002752492f, 	0.001060269f, 	0.0f,
    0.002556406f, 	0.0009848854f, 	0.0f,
    0.002374564f, 	0.0009149703f, 	0.0f,
    0.002205568f, 	0.0008499903f, 	0.0f,
    0.002048294f, 	0.0007895158f, 	0.0f,
    0.001902113f, 	0.0007333038f, 	0.0f,
    0.001766485f, 	0.0006811458f, 	0.0f,
    0.001640857f, 	0.0006328287f, 	0.0f,
    0.001524672f, 	0.0005881375f, 	0.0f,
    0.001417322f, 	0.0005468389f, 	0.0f,
    0.001318031f, 	0.0005086349f, 	0.0f,
    0.001226059f, 	0.0004732403f, 	0.0f,
    0.001140743f, 	0.0004404016f, 	0.0f,
    0.001061495f, 	0.0004098928f, 	0.0f,
    0.0009877949f, 	0.0003815137f, 	0.0f,
    0.0009191847f, 	0.0003550902f, 	0.0f,
    0.0008552568f, 	0.0003304668f, 	0.0f,
    0.0007956433f, 	0.000307503f, 	0.0f,
    0.000740012f, 	0.0002860718f, 	0.0f,
    0.000688098f, 	0.0002660718f, 	0.0f,
    0.0006397864f, 	0.0002474586f, 	0.0f,
    0.0005949726f, 	0.0002301919f, 	0.0f,
    0.0005535291f, 	0.0002142225f, 	0.0f,
    0.0005153113f, 	0.0001994949f, 	0.0f,
    0.0004801234f, 	0.0001859336f, 	0.0f,
    0.0004476245f, 	0.0001734067f, 	0.0f,
    0.0004174846f, 	0.0001617865f, 	0.0f,
    0.0003894221f, 	0.0001509641f, 	0.0f,
    0.0003631969f, 	0.0001408466f, 	0.0f,
    0.0003386279f, 	0.0001313642f, 	0.0f,
    0.0003156452f, 	0.0001224905f, 	0.0f,
    0.0002941966f, 	0.000114206f, 	0.0f,
    0.0002742235f, 	0.0001064886f, 	0.0f,
    0.0002556624f, 	0.00009931439f, 	0.0f,
    0.000238439f, 	0.00009265512f, 	0.0f,
    0.0002224525f, 	0.00008647225f, 	0.0f,
    0.0002076036f, 	0.0000807278f, 	0.0f,
    0.0001938018f, 	0.00007538716f, 	0.0f,
    0.0001809649f, 	0.00007041878f, 	0.0f,
    0.0001690167f, 	0.00006579338f, 	0.0f,
    0.0001578839f, 	0.0000614825f, 	0.0f,
    0.0001474993f, 	0.00005746008f, 	0.0f,
    0.0001378026f, 	0.00005370272f, 	0.0f,
    0.0001287394f, 	0.00005018934f, 	0.0f,
    0.0001202644f, 	0.00004690245f, 	0.0f,
    0.0001123502f, 	0.00004383167f, 	0.0f,
    0.0001049725f, 	0.0000409678f, 	0.0f,
    0.00009810596f, 	0.00003830123f, 	0.0f,
    0.00009172477f, 	0.00003582218f, 	0.0f,
    0.00008579861f, 	0.00003351903f, 	0.0f,
    0.00008028174f, 	0.00003137419f, 	0.0f,
    0.00007513013f, 	0.00002937068f, 	0.0f,
    0.00007030565f, 	0.0000274938f, 	0.0f,
    0.00006577532f, 	0.00002573083f, 	0.0f,
    0.00006151508f, 	0.00002407249f, 	0.0f,
    0.00005752025f, 	0.00002251704f, 	0.0f,
    0.00005378813f, 	0.0000210635f, 	0.0f,
    0.0000503135f, 	0.00001970991f, 	0.0f,
    0.00004708916f, 	0.00001845353f, 	0.0f,
    0.00004410322f, 	0.00001728979f, 	0.0f,
    0.0000413315f, 	0.00001620928f, 	0.0f,
    0.00003874992f, 	0.00001520262f, 	0.0f,
    0.00003633762f, 	0.00001426169f, 	0.0f,
    0.00003407653f, 	0.00001337946f, 	0.0f,
    0.00003195242f, 	0.00001255038f, 	0.0f,
    0.00002995808f, 	0.00001177169f, 	0.0f,
    0.00002808781f, 	0.00001104118f, 	0.0f,
    0.00002633581f, 	0.00001035662f, 	0.0f,
    0.0000246963f, 	0.000009715798f, 	0.0f,
    0.00002316311f, 	0.000009116316f, 	0.0f,
    0.00002172855f, 	0.000008555201f, 	0.0f,
    0.00002038519f, 	0.000008029561f, 	0.0f,
    0.00001912625f, 	0.000007536768f, 	0.0f,
    0.00001794555f, 	0.000007074424f, 	0.0f,
    0.00001683776f, 	0.000006640464f, 	0.0f,
    0.00001579907f, 	0.000006233437f, 	0.0f,
    0.00001482604f, 	0.000005852035f, 	0.0f,
    0.00001391527f, 	0.000005494963f, 	0.0f,
    0.00001306345f, 	0.000005160948f, 	0.0f,
    0.0000122672f, 	0.000004848687f, 	0.0f,
    0.00001152279f, 	0.000004556705f, 	0.0f,
    0.00001082663f, 	0.00000428358f, 	0.0f,
    0.0000101754f, 	0.000004027993f, 	0.0f,
    0.000009565993f, 	0.000003788729f, 	0.0f,
    0.000008995405f, 	0.000003564599f, 	0.0f,
    0.000008460253f, 	0.000003354285f, 	0.0f,
    0.000007957382f, 	0.000003156557f, 	0.0f,
    0.000007483997f, 	0.000002970326f, 	0.0f,
    0.000007037621f, 	0.000002794625f, 	0.0f,
    0.000006616311f, 	0.000002628701f, 	0.0f,
    0.000006219265f, 	0.000002472248f, 	0.0f,
    0.000005845844f, 	0.00000232503f, 	0.0f,
    0.000005495311f, 	0.000002186768f, 	0.0f,
    0.000005166853f, 	0.000002057152f, 	0.0f,
    0.000004859511f, 	0.000001935813f, 	0.0f,
    0.000004571973f, 	0.000001822239f, 	0.0f,
    0.00000430292f, 	0.000001715914f, 	0.0f,
    0.000004051121f, 	0.000001616355f, 	0.0f,
    0.000003815429f, 	0.000001523114f, 	0.0f,
    0.000003594719f, 	0.00000143575f, 	0.0f,
    0.000003387736f, 	0.000001353771f, 	0.0f,
    0.000003193301f, 	0.000001276714f, 	0.0f,
    0.000003010363f, 	0.000001204166f, 	0.0f,
    0.00000283798f, 	0.000001135758f, 	0.0f,
    0.000002675365f, 	0.000001071181f, 	0.0f,
    0.00000252202f, 	0.000001010243f, 	0.0f,
    0.000002377511f, 	0.0000009527779f, 	0.0f,
    0.000002241417f, 	0.0000008986224f, 	0.0f,
    0.000002113325f, 	0.0000008476168f, 	0.0f,
    0.00000199283f, 	0.0000007996052f, 	0.0f,
    0.000001879542f, 	0.0000007544361f, 	0.0f,
    0.000001773083f, 	0.0000007119624f, 	0.0f,
    0.000001673086f, 	0.0000006720421f, 	0.0f,
    0.000001579199f, 	0.000000634538f, 	0.0f
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

void world1(Camera& camera, std::vector<sphere>& spheres, std::vector<plane>& planes, std::vector<box>& boxes, std::vector<lens>& lenses, std::vector<material>& materials) {
	// Camera
	camera.pos = glm::vec3(6.332f, 3.855f, 3.140f);
	camera.angle = glm::vec2(225.093f, -31.512f);
	camera.ISO = 1600;
	camera.size = 0.057f;
	camera.apertureSize = 0.0025f;
	camera.apertureDist = 0.049f;
	camera.lensRadius = 0.0100f;
	camera.lensFocalLength = 0.0300f;
	camera.lensThickness = 0.0000f;
	camera.lensDistance = 0.050f;

	// Spheres
	sphere sphere1 = { { 0.0f, 1.0f, 0.0f }, 1.0f, 1 };
	sphere sphere2 = { { 5.0f, 1.0f, -1.0f }, 1.0f, 2 };
	sphere sphere3 = { { 0.0f, 4.0f, -3.0f }, 1.0f, 3 };
	spheres.push_back(sphere1);
	spheres.push_back(sphere2);
	spheres.push_back(sphere3);

	// Planes
	plane plane1 = { { 0.0f, 0.0f, 0.0f }, 1 };
	planes.push_back(plane1);

	// Boxes
	box box1 = { { 3.0f, 0.75f, 1.0f }, { 0.0f, 58.31f, 0.0f }, { 1.5f, 1.5f, 1.5f }, 1 };
	boxes.push_back(box1);
	
	// Lenses
	lens lens1 = { { 5.0f, 1.2f, -4.0f }, { 0.0f, 0.0f, 0.0f }, 1.2f, 1.0f, 0.0f, true, 1 };
	lenses.push_back(lens1);

	// Materials
	material material1 = { { 550.0f, 100.0f, 0 }, { 5500.0f, 0.0f } };
	material material2 = { { 470.0f, 6.0f, 0 }, { 5500.0f, 0.0f } };
	material material3 = { { 550.0f, 0.0f, 0 }, { 5500.0f, 12.5f } };
	materials.push_back(material1);
	materials.push_back(material2);
	materials.push_back(material3);
}

float SpectralPowerDistribution(float l, float l_peak, float d, float invert) {
	// Spectral Power Distribution Function Calculated On The Basis Of Peak Wavelength And Standard Deviation
	// Using Gaussian Function To Predict Spectral Radiance
	// In Reality, Spectral Radiance Function Has Different Shapes For Different Objects Also Looks Much Different Than This
	float x = (l - l_peak) / (2.0f * d * d);
	float radiance = exp(-x * x);
	radiance = glm::mix(radiance, 1.0f - radiance, invert);

	return radiance;
}

float BlackBodyRadiation(float l, float T) {
	// Plank's Law
	return (1.1910429724e-16f * pow(l, -5.0f)) / (exp(0.014387768775f / (l * T)) - 1.0f);
}

float BlackBodyRadiationPeak(float T) {
	// Derived By Substituting Wien's Displacement Law On Plank's Law
	return 4.0956746759e-6f * pow(T, 5.0f);
}

float Reinhard(float x) {
	// x / (1 + x)
	return x / (1.0f + x);
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float ACESFilm(float x) {
	// x(ax + b) / (x(cx + d) + e)
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return x * (a * x + b) / (x * (c * x + d) + e);
}

// DEUCES Biophotometric Tonemap by Ted(Kerdek)
float DEUCESBioPhotometric(float x) {
	// e^(-0.25 / x)
	return exp(-0.25f / x);
}

float tonemapping(float x, int tonemap) {
	if (tonemap == 1) {
		x = Reinhard(x);
	}

	if (tonemap == 2) {
		x = ACESFilm(x);
	}

	if (tonemap == 3) {
		x = DEUCESBioPhotometric(x);
	}

	return x;
}


class App {
public:
    void run() {
        InitWindow();
        InitVulkan();
		if (!OFFSCREENRENDER) {
			InitImGui();
		}
        MainLoop();
        CleanUp();
    }
private:
    SDL_Window* window;
	int W = WIDTH;
	int H = HEIGHT;
	bool VSync = false;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkShaderModule> shaderModules;

	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;

	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;

	VkCommandPool commandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	UniformBufferObject ubo;

	VkBuffer texelBuffer;
	VkDeviceMemory texelBufferMemory;
	VkFormat texelBufferFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	VkBufferView texelBufferView;

	VkImage processorImage;
	VkFormat processorImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkDeviceMemory processorImageMemory;
	VkImageView processorImageView;

	VkImage saveImage;
	VkFormat saveImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkDeviceMemory saveImageMemory;

	std::vector<VkFramebuffer> framebuffers;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	PushConstantValues pushConstant;

	std::vector<VkCommandBuffer> graphicsCommandBuffers;
	std::vector<VkCommandBuffer> computeCommandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<VkSemaphore> computeFinishedSemaphores;
	std::vector<VkFence> computeInFlightFences;

	VkDescriptorPool imguiDescriptorPool;
	ImGuiIO* io;

	bool isViewPortResized = false;
	bool isWindowMinimized = false;
	bool isVSyncChanged = false;
	bool isReset = false;
	bool isUpdateUBO = true;

	uint32_t currentFrame = 0;
	int samplesPerFrame = 1;
	int frame = samplesPerFrame;
	int samples = samplesPerFrame;
	float FPS = 60.0f;

	float persistence = 0.0625f;
	int pathLength = 5;
	int tonemap = TONEMAP;

	std::vector<sphere> spheres;
	std::vector<plane> planes;
	std::vector<box> boxes;
	std::vector<lens> lenses;
	std::vector<material> materials;
	Camera camera{};

    void InitWindow() {
        SDL_Init(SDL_INIT_VIDEO);

        Uint32 WindowFlags = SDL_WINDOW_VULKAN;

		if (OFFSCREENRENDER) {
			WindowFlags |= SDL_WINDOW_HIDDEN;
		} else {
			WindowFlags |= SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
		}

        window = SDL_CreateWindow("Path Tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, WindowFlags);
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
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
				(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
				indices.graphicsComputeFamily = i;
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
			score = 100;
		}
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
			score = 300;
		}
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score = 1000;
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
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsComputeFamily.value(), indices.presentFamily.value()};

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

		VkPhysicalDeviceUniformBufferStandardLayoutFeatures UBOLayoutFeatures{};
		UBOLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
		UBOLayoutFeatures.uniformBufferStandardLayout = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &UBOLayoutFeatures;
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

		vkGetDeviceQueue(device, indices.graphicsComputeFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.graphicsComputeFamily.value(), 0, &computeQueue);
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
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
				surfaceFormat = availableFormat;
				for (const colorSpaceName& colorSpace : colorSpaces) {
					if (surfaceFormat.colorSpace == colorSpace.space) {
						std::cout << "Using VK_FORMAT_B8G8R8A8_UNORM Format With " << colorSpace.name << " Color Space" << std::endl;
						return surfaceFormat;
					}
				}
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

		if (VSync) {
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

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 2;
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

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsComputeFamily.value(), indices.presentFamily.value()};
		
		if (indices.graphicsComputeFamily != indices.presentFamily) {
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
	}

	void CreateSwapChainImageViews() {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			createInfo.image = swapChainImages[i];
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
		std::vector<VkAttachmentDescription> colorAttachments{};
		VkAttachmentReference subpass1ColorAttachmentRefs{};
		VkAttachmentReference subpass2ColorAttachmentRef{};
		std::vector<VkSubpassDescription> subpass{};
		std::vector<VkSubpassDependency> dependency{};
		VkRenderPassCreateInfo createInfo{};

		if (OFFSCREENRENDER) {
			colorAttachments.resize(1);
			subpass.resize(1);
			dependency.resize(1);
		} else {
			colorAttachments.resize(2);
			subpass.resize(2);
			dependency.resize(2);
		}

		if (OFFSCREENRENDER) {
			colorAttachments[0].format = processorImageFormat;
			colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		} else {
			colorAttachments[0].format = swapChainImageFormat;
			colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			colorAttachments[1].format = swapChainImageFormat;
			colorAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		subpass1ColorAttachmentRefs.attachment = 0;
		subpass1ColorAttachmentRefs.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass2ColorAttachmentRef.attachment = 1;
		subpass2ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass[0].colorAttachmentCount = 1;
		subpass[0].pColorAttachments = &subpass1ColorAttachmentRefs;

		if (!OFFSCREENRENDER) {
			subpass[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass[1].colorAttachmentCount = 1;
			subpass[1].pColorAttachments = &subpass2ColorAttachmentRef;
		}

		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[0].dstSubpass = 0;
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency[0].srcAccessMask = 0;
		dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		if (!OFFSCREENRENDER) {
			dependency[1].srcSubpass = 0;
			dependency[1].dstSubpass = 1;
			dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		}

		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = static_cast<uint32_t>(colorAttachments.size());
		createInfo.pAttachments = colorAttachments.data();
		createInfo.subpassCount = static_cast<uint32_t>(subpass.size());
		createInfo.pSubpasses = subpass.data();
		createInfo.dependencyCount = static_cast<uint32_t>(dependency.size());
		createInfo.pDependencies = dependency.data();

		if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Render Pass!");
		}
	}

	void CreateDescriptorSetLayout() {
		std::array<VkDescriptorSetLayoutBinding, 2> layoutBinding{};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};

		layoutBinding[0].binding = 0;
		layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding[0].descriptorCount = 1;
		layoutBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBinding[0].pImmutableSamplers = nullptr;

		layoutBinding[1].binding = 1;
		layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		layoutBinding[1].descriptorCount = 1;
		layoutBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding[1].pImmutableSamplers = nullptr;

		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
		layoutInfo.pBindings = layoutBinding.data();
		layoutInfo.flags = 0;

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

	VkPipelineShaderStageCreateInfo CreateShaderStageInfo(VkShaderModule module, VkShaderStageFlagBits stage, const char* pName) {
		shaderModules.push_back(module);

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = stage;
		shaderStageInfo.module = module;
		shaderStageInfo.pName = pName;
		shaderStageInfo.pSpecializationInfo = nullptr; // For Specifying Different Constants Instead Of Doing It In Runtime

		return shaderStageInfo;
	}

	void CreateGraphicsPipeline() {
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = CreateShaderStageInfo(CreateShaderModule(ReadFile("vertex.spv")), VK_SHADER_STAGE_VERTEX_BIT, "main");
		shaderStages[1] = CreateShaderStageInfo(CreateShaderModule(ReadFile("fragment.spv")), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

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

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantValues);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// Uniforms And Push Values
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Graphics Pipeline Layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.layout = graphicsPipelineLayout;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Graphics Pipeline!");
		}
	}

	void CreateComputePipeline() {
		VkPipelineShaderStageCreateInfo computeShaderStage{};
		computeShaderStage = CreateShaderStageInfo(CreateShaderModule(ReadFile("compute.spv")), VK_SHADER_STAGE_COMPUTE_BIT, "main");

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantValues);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		// Uniforms And Push Values
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Compute Pipeline Layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = computePipelineLayout;
		pipelineInfo.stage = computeShaderStage;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Compute Pipeline!");
		}
	}

	void CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsComputeFamily.value();

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

	void CreateUniformBuffer() {
		for (int i = 0; i < MAX_OBJECTS_SIZE; i++) {
			ubo.packedObjects[i] = 0.0f;
		}

		for (int i = 0; i < MAX_MATERIALS_SIZE; i++) {
			ubo.packedMaterials[i] = 0.0f;
		}

		for (int i = 0; i < std::size(CIEXYZ2006); i++) {
			ubo.CIEXYZ2006[i] = CIEXYZ2006[i];
		}

		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void CreateTexelBuffer() {
		VkDeviceSize bufferSize = W * H * 4 * 4;

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texelBuffer, texelBufferMemory);
	}

	void CreateTexelBufferView() {
		VkBufferViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.buffer = texelBuffer;
		createInfo.format = texelBufferFormat;
		createInfo.offset = 0;
		createInfo.range = VK_WHOLE_SIZE;

		if (vkCreateBufferView(device, &createInfo, nullptr, &texelBufferView) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Texel Buffer View!");
		}
	}

	void CreateImages() {
		if (OFFSCREENRENDER) {
			std::array<VkImageCreateInfo, 2> createInfo{};
			std::array<VkMemoryRequirements, 2> memoryRequirements;
			std::array<VkMemoryAllocateInfo, 2> allocateInfo{};

			createInfo[0].sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo[0].imageType = VK_IMAGE_TYPE_2D;
			createInfo[0].extent.width = static_cast<uint32_t>(W);
			createInfo[0].extent.height = static_cast<uint32_t>(H);
			createInfo[0].extent.depth = 1;
			createInfo[0].mipLevels = 1;
			createInfo[0].arrayLayers = 1;
			createInfo[0].format = processorImageFormat;
			createInfo[0].tiling = VK_IMAGE_TILING_OPTIMAL;
			createInfo[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			createInfo[0].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			createInfo[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo[0].samples = VK_SAMPLE_COUNT_1_BIT;
			createInfo[0].flags = 0;

			if (vkCreateImage(device, &createInfo[0], nullptr, &processorImage) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Create Processor Image!");
			}

			vkGetImageMemoryRequirements(device, processorImage, &memoryRequirements[0]);

			allocateInfo[0].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo[0].allocationSize = memoryRequirements[0].size;
			allocateInfo[0].memoryTypeIndex = FindMemoryType(memoryRequirements[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (vkAllocateMemory(device, &allocateInfo[0], nullptr, &processorImageMemory) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Allocate Processor Image Memory!");
			}

			vkBindImageMemory(device, processorImage, processorImageMemory, 0);

			createInfo[1].sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo[1].imageType = VK_IMAGE_TYPE_2D;
			createInfo[1].extent.width = static_cast<uint32_t>(W);
			createInfo[1].extent.height = static_cast<uint32_t>(H);
			createInfo[1].extent.depth = 1;
			createInfo[1].mipLevels = 1;
			createInfo[1].arrayLayers = 1;
			createInfo[1].format = saveImageFormat;
			createInfo[1].tiling = VK_IMAGE_TILING_LINEAR;
			createInfo[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			createInfo[1].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			createInfo[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo[1].samples = VK_SAMPLE_COUNT_1_BIT;
			createInfo[1].flags = 0;

			if (vkCreateImage(device, &createInfo[1], nullptr, &saveImage) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Create Save Image!");
			}

			vkGetImageMemoryRequirements(device, saveImage, &memoryRequirements[1]);

			allocateInfo[1].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo[1].allocationSize = memoryRequirements[1].size;
			allocateInfo[1].memoryTypeIndex = FindMemoryType(memoryRequirements[1].memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			if (vkAllocateMemory(device, &allocateInfo[1], nullptr, &saveImageMemory) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Allocate Save Image Memory!");
			}

			vkBindImageMemory(device, saveImage, saveImageMemory, 0);
		}
	}

	void CreateImageViews() {
		VkImageViewCreateInfo createInfo{};

		if (OFFSCREENRENDER) {
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = processorImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.image = processorImage;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &processorImageView) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Create Processor Image View!");
			}
		}
	}

	void CreateFramebuffers() {
		if (OFFSCREENRENDER) {
			framebuffers.resize(1);
		} else {
			framebuffers.resize(swapChainImageViews.size());
		}

		for (size_t i = 0; i < framebuffers.size(); i++) {
			std::vector<VkImageView> attachments;
			if (OFFSCREENRENDER) {
				attachments.push_back(processorImageView);
			} else {
				attachments.push_back(swapChainImageViews[i]);
				attachments.push_back(swapChainImageViews[i]);
			}

			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = renderPass;
			createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			createInfo.pAttachments = attachments.data();
			createInfo.width = static_cast<uint32_t>(W);
			createInfo.height = static_cast<uint32_t>(H);
			createInfo.layers = 1;

			if (vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
				std::runtime_error("Failed To Create Framebuffers!");
			}
		}
	}

	void CreateDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSize{};
		VkDescriptorPoolCreateInfo poolInfo{};

		poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		poolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSize[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Descriptor Pool!");
		}
	}

	void UpdateDescriptorSet() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			std::array<VkWriteDescriptorSet, 2> descriptorWrite{};

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = descriptorSets[i];
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite[0].descriptorCount = 1;
			descriptorWrite[0].pBufferInfo = &bufferInfo;
			descriptorWrite[0].pImageInfo = nullptr;
			descriptorWrite[0].pTexelBufferView = nullptr;

			VkDescriptorBufferInfo texelBufferInfo{};
			texelBufferInfo.buffer = texelBuffer;
			texelBufferInfo.offset = 0;
			texelBufferInfo.range = W * H * 4 * 4;

			descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[1].dstSet = descriptorSets[i];
			descriptorWrite[1].dstBinding = 1;
			descriptorWrite[1].dstArrayElement = 0;
			descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			descriptorWrite[1].descriptorCount = 1;
			descriptorWrite[1].pBufferInfo = &texelBufferInfo;
			descriptorWrite[1].pImageInfo = nullptr;
			descriptorWrite[1].pTexelBufferView = &texelBufferView;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrite.size()), descriptorWrite.data(), 0, nullptr);
		}
	}

	void CreateDescriptorSet() {
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		allocateInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Descriptor Sets!");
		}

		UpdateDescriptorSet();
	}

	void CreateCommandBuffer() {
		graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		std::array<VkCommandBufferAllocateInfo, 2> allocateInfo{};

		allocateInfo[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo[0].commandPool = commandPool;
		allocateInfo[0].level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo[0].commandBufferCount = static_cast<uint32_t>(graphicsCommandBuffers.size());

		allocateInfo[1].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo[1].commandPool = commandPool;
		allocateInfo[1].level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo[1].commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

		if (vkAllocateCommandBuffers(device, &allocateInfo[0], graphicsCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Graphics Command Buffers!");
		}

		if (vkAllocateCommandBuffers(device, &allocateInfo[1], computeCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Compute Command Buffers!");
		}
	}

	void CreateSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if ((vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) || 
			(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) || 
			(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)) {
				throw std::runtime_error("Failed To Create Graphics Sync Objects For A Frame!");
			}

			if ((vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS) || 
			(vkCreateFence(device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS)) {
				throw std::runtime_error("Failed To Create Compute Sync Objects For A Frame!");
			}
		}
	}

    void InitVulkan() {
        CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		if (!OFFSCREENRENDER) {
			CreateSwapChain();
			CreateSwapChainImageViews();
		}
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateComputePipeline();
		CreateCommandPool();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffer();
		CreateTexelBuffer();
		CreateTexelBufferView();
		CreateImages();
		CreateImageViews();
		CreateFramebuffers();
		CreateDescriptorPool();
		CreateDescriptorSet();
		CreateCommandBuffer();
		CreateSyncObjects();
    }

	void InitImGui() {
		VkDescriptorPoolSize poolSize[] = 
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = std::size(poolSize);
		poolInfo.pPoolSizes = poolSize;
		poolInfo.maxSets = 1000;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create ImGui Descriptor Pool!");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		io = &ImGui::GetIO(); (void)io;
		io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		ImGui::StyleColorsDark();

		ImGui_ImplSDL2_InitForVulkan(window);

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.Queue = graphicsQueue;
		initInfo.QueueFamily = FindQueueFamilies(physicalDevice).graphicsComputeFamily.value();
		initInfo.DescriptorPool = imguiDescriptorPool;
		initInfo.MinImageCount = static_cast<uint32_t>(swapChainImageViews.size());
		initInfo.ImageCount = static_cast<uint32_t>(swapChainImageViews.size());
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.RenderPass = renderPass;
		initInfo.Subpass = 1;
		initInfo.UseDynamicRendering = false;

		ImGui_ImplVulkan_Init(&initInfo);

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	void SDLHandleEvents(bool& isRunning, glm::vec2& cursorPos, glm::vec2& cameraAngle, glm::vec3& deltaCamPos, bool& isWindowFocused) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                isRunning = false;
				return;
            }

			if ((event.window.event == SDL_WINDOWEVENT_RESIZED) && (!isViewPortResized)) {
				isViewPortResized = true;
				break;
			}
        }

		glm::vec2 cursorPos1 = glm::vec2((float)(ImGui::GetMousePos().x) / (float)W, (float)(H - ImGui::GetMousePos().y) / (float)H);

		if (!isWindowFocused) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				isReset = true;
				glm::vec2 dxdy = cursorPos1 - cursorPos;
				cameraAngle = cameraAngle - (360.0f * dxdy);
				if (cameraAngle.x > 360.0f) {
					cameraAngle.x = cameraAngle.x - 360.0f;
				}
				if (cameraAngle.x < 0.0f) {
					cameraAngle.x = 360.0f + cameraAngle.x;
				}
				if (cameraAngle.y > 90.0f) {
					cameraAngle.y = 90.0f;
				}
				if (cameraAngle.y < -90.0f) {
					cameraAngle.y = -90.0f;
				}
			}
		}

		cursorPos = cursorPos1;

		deltaCamPos.x = ((float)(ImGui::IsKeyDown(ImGuiKey_D)) - (float)(ImGui::IsKeyDown(ImGuiKey_A)));
		deltaCamPos.y = ((float)(ImGui::IsKeyDown(ImGuiKey_E)) - (float)(ImGui::IsKeyDown(ImGuiKey_Q)));
		deltaCamPos.z = ((float)(ImGui::IsKeyDown(ImGuiKey_W)) - (float)(ImGui::IsKeyDown(ImGuiKey_S)));

		if ((deltaCamPos.x != 0.0f) || (deltaCamPos.y != 0.0f) || (deltaCamPos.z != 0.0f)) {
			isReset = true;
		}
    }

	void UpdateCameraPos(glm::vec3& cameraPos, glm::vec2 cameraAngle, glm::vec3 deltaCamPos) {
		// http://www.songho.ca/opengl/gl_anglestoaxes.html
		glm::vec2 theta = glm::vec2(-cameraAngle.y, cameraAngle.x);
		glm::mat3 mX = glm::mat3(1.0f, 0.0f, 0.0f, 0.0f, glm::cos(theta.x), -glm::sin(theta.x), 0.0f, glm::sin(theta.x), glm::cos(theta.x));
		glm::mat3 mY = glm::mat3(glm::cos(theta.y), 0.0f, glm::sin(theta.y), 0.0f, 1.0f, 0.0f, -glm::sin(theta.y), 0.0f, glm::cos(theta.y));
		glm::mat3 m = mX * mY;

		cameraPos = cameraPos + deltaCamPos * m;
	}

	void ItemsTable(const char* name, int& selection, int id, int size) {
		for (int i = id; i < (size + id); i++) {
			ImGui::PushID(i);
			ImGui::TableNextRow();
			char label[32];
			sprintf_s(label, name, i - id + 1);
			ImGui::TableSetColumnIndex(0);
			bool isSelected = selection == i;

			if (ImGui::Selectable(label, &isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
				if (isSelected) {
					selection = i;
				}
			}

			ImGui::PopID();
		}
	}

	void RecordGraphicsCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Begin Recording Graphics Command Buffer!");
		}

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(W);
		viewport.height = static_cast<float>(H);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent.width = static_cast<uint32_t>(W);
		scissor.extent.height = static_cast<uint32_t>(H);

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
		renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(W);
		renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(H);
		renderPassBeginInfo.renderArea.offset = {0, 0};
		renderPassBeginInfo.clearValueCount = 0;
		renderPassBeginInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		graphicsPipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstant), &pushConstant);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		if (!OFFSCREENRENDER) {
			vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
		}

		vkCmdEndRenderPass(commandBuffer);

		if (OFFSCREENRENDER) {
			if (samples >= NUMSAMPLES) {
				std::array<VkImageMemoryBarrier, 2> imageMemoryBarrierTransfer1{};

				imageMemoryBarrierTransfer1[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrierTransfer1[0].image = processorImage;
				imageMemoryBarrierTransfer1[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrierTransfer1[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrierTransfer1[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageMemoryBarrierTransfer1[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrierTransfer1[0].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				imageMemoryBarrierTransfer1[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrierTransfer1[1].image = saveImage;
				imageMemoryBarrierTransfer1[1].srcAccessMask = 0;
				imageMemoryBarrierTransfer1[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrierTransfer1[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrierTransfer1[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrierTransfer1[1].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, imageMemoryBarrierTransfer1.data());

				VkImageCopy region{};
				region.extent.width = static_cast<uint32_t>(W);
				region.extent.height = static_cast<uint32_t>(H);
				region.extent.depth = 1;
				region.srcOffset = {0, 0, 0};
				region.dstOffset = {0, 0, 0};
				region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
				region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

				VkClearColorValue clearColor = {{0.0f, 1.0f, 0.0f, 1.0f}};

				VkImageSubresourceRange range{};
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseArrayLayer = 0;
				range.layerCount = 1;
				range.baseMipLevel = 0;
				range.levelCount = 1;

				vkCmdCopyImage(commandBuffer, processorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, saveImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				std::array<VkImageMemoryBarrier, 2> imageMemoryBarrierTransfer2{};

				imageMemoryBarrierTransfer2[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrierTransfer2[0].image = processorImage;
				imageMemoryBarrierTransfer2[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrierTransfer2[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrierTransfer2[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrierTransfer2[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageMemoryBarrierTransfer2[0].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				imageMemoryBarrierTransfer2[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrierTransfer2[1].image = saveImage;
				imageMemoryBarrierTransfer2[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrierTransfer2[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				imageMemoryBarrierTransfer2[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrierTransfer2[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrierTransfer2[1].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, imageMemoryBarrierTransfer2.data());
			}
		}

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Record Graphics Command Buffer!");
		}
	}

	void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Begin Recording Compute Command Buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
		computePipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstant), &pushConstant);

		vkCmdDispatch(commandBuffer, static_cast<uint32_t>(std::ceil(W / 16.0)), static_cast<uint32_t>(std::ceil(H / 16.0)), 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Record Compute Command Buffer!");
		}
	}

	void CleanUpImages() {
		for (VkFramebuffer framebuffer : framebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		if (OFFSCREENRENDER) {
			vkDestroyImage(device, saveImage, nullptr);
			vkFreeMemory(device, saveImageMemory, nullptr);
			vkDestroyImageView(device, processorImageView, nullptr);
			vkDestroyImage(device, processorImage, nullptr);
			vkFreeMemory(device, processorImageMemory, nullptr);
		} else {
			for (VkImageView imageView : swapChainImageViews) {
				vkDestroyImageView(device, imageView, nullptr);
			}

			vkDestroySwapchainKHR(device, swapChain, nullptr);
		}
	}

	void CleanUpTexelBuffer() {
		vkDestroyBufferView(device, texelBufferView, nullptr);
		vkDestroyBuffer(device, texelBuffer, nullptr);
		vkFreeMemory(device, texelBufferMemory, nullptr);
	}

	void RecreateSwapChain() {
		vkDeviceWaitIdle(device);

		CleanUpImages();

		CreateSwapChain();
		CreateSwapChainImageViews();

		CreateImages();
		CreateImageViews();

		CreateFramebuffers();
	}

	void RecreateImages() {
		SDL_Event event;

		while ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0) {
			SDL_WaitEvent(&event);

			isWindowMinimized = true;
		}

		if (isWindowMinimized) {
			return;
		}

		RecreateSwapChain();

		CleanUpTexelBuffer();

		CreateTexelBuffer();
		CreateTexelBufferView();

		UpdateDescriptorSet();
	}

	void UpdateUniformBuffer() {
		if (isUpdateUBO) {
			std::array<float, 4> numObjects;
			std::vector<float> objectsArray;
			std::vector<float> materialsArray;

			numObjects[0] = (float)spheres.size();
			numObjects[1] = (float)planes.size();
			numObjects[2] = (float)boxes.size();
			numObjects[3] = (float)lenses.size();

			for (size_t i = 0; i < numObjects.size(); i++) {
				ubo.numObjects[i] = numObjects[i];
			}

			for (int i = 0; i < spheres.size(); i++) {
				objectsArray.push_back(spheres[i].pos[0]);
				objectsArray.push_back(spheres[i].pos[1]);
				objectsArray.push_back(spheres[i].pos[2]);
				objectsArray.push_back(spheres[i].radius);
				objectsArray.push_back((float)spheres[i].materialID);
			}

			for (int i = 0; i < planes.size(); i++) {
				objectsArray.push_back(planes[i].pos[0]);
				objectsArray.push_back(planes[i].pos[1]);
				objectsArray.push_back(planes[i].pos[2]);
				objectsArray.push_back((float)planes[i].materialID);
			}

			for (int i = 0; i < boxes.size(); i++) {
				objectsArray.push_back(boxes[i].pos[0]);
				objectsArray.push_back(boxes[i].pos[1]);
				objectsArray.push_back(boxes[i].pos[2]);
				objectsArray.push_back(boxes[i].rotation[0]);
				objectsArray.push_back(boxes[i].rotation[1]);
				objectsArray.push_back(boxes[i].rotation[2]);
				objectsArray.push_back(boxes[i].size[0]);
				objectsArray.push_back(boxes[i].size[1]);
				objectsArray.push_back(boxes[i].size[2]);
				objectsArray.push_back((float)boxes[i].materialID);
			}

			for (int i = 0; i < lenses.size(); i++) {
				objectsArray.push_back(lenses[i].pos[0]);
				objectsArray.push_back(lenses[i].pos[1]);
				objectsArray.push_back(lenses[i].pos[2]);
				objectsArray.push_back(lenses[i].rotation[0]);
				objectsArray.push_back(lenses[i].rotation[1]);
				objectsArray.push_back(lenses[i].rotation[2]);
				objectsArray.push_back(lenses[i].radius);
				objectsArray.push_back(lenses[i].focalLength);
				objectsArray.push_back(lenses[i].thickness);
				objectsArray.push_back((float)lenses[i].isConverging);
				objectsArray.push_back((float)lenses[i].materialID);
			}

			for (int i = 0; i < MAX_OBJECTS_SIZE; i++) {
				if (objectsArray.size() > i) {
					ubo.packedObjects[i] = objectsArray[i];
				}
			}

			for (int i = 0; i < materials.size(); i++) {
				materialsArray.push_back(materials[i].reflection[0]);
				materialsArray.push_back(materials[i].reflection[1]);
				materialsArray.push_back(materials[i].reflection[2]);
				materialsArray.push_back(materials[i].emission[0]);
				materialsArray.push_back(materials[i].emission[1]);
			}

			for (int i = 0; i < MAX_MATERIALS_SIZE; i++) {
				if (materialsArray.size() > i) {
					ubo.packedMaterials[i] = materialsArray[i];
				}
			}

			for (size_t k = 0; k < MAX_FRAMES_IN_FLIGHT; k++) {
				memcpy(uniformBuffersMapped[k], &ubo, sizeof(ubo));
			}

			isUpdateUBO = false;
		}
	}

	void UpdatePushConstant() {
		pushConstant.resolution = glm::ivec2(W, H);
		pushConstant.frame = frame;
		pushConstant.samples = samples;
		pushConstant.samplesPerFrame = samplesPerFrame;
		pushConstant.FPS = FPS;
		pushConstant.persistence = persistence;
		pushConstant.pathLength = pathLength;
		pushConstant.cameraAngle = glm::vec2(-camera.angle.y, camera.angle.x);
		pushConstant.cameraPosX = camera.pos.x;
		pushConstant.cameraPosY = camera.pos.y;
		pushConstant.cameraPosZ = camera.pos.z;
		pushConstant.ISO = camera.ISO;
		pushConstant.cameraSize = camera.size;
		pushConstant.apertureSize = camera.apertureSize;
		pushConstant.apertureDist = camera.apertureDist;
		pushConstant.lensRadius = camera.lensRadius;
		pushConstant.lensFocalLength = camera.lensFocalLength;
		pushConstant.lensThickness = camera.lensThickness;
		pushConstant.lensDistance = camera.lensDistance;
		pushConstant.tonemap = tonemap;
	}

	void DrawFrame() {
		bool isGraphicsRender = (!OFFSCREENRENDER) || (OFFSCREENRENDER && (samples >= NUMSAMPLES));

		vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		UpdateUniformBuffer();
		UpdatePushConstant();

		vkResetFences(device, 1, &computeInFlightFences[currentFrame]);

		vkResetCommandBuffer(computeCommandBuffers[currentFrame], 0);
		RecordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

		VkSubmitInfo computeSubmitInfo{};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];

		if (isGraphicsRender) {
			computeSubmitInfo.signalSemaphoreCount = 1;
			computeSubmitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];
		}

		if (vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Submit Compute Command Buffers!");
		}

		if (isGraphicsRender) { 
			vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		}

		uint32_t imageIndex = 0;
		VkResult result;

		if (!OFFSCREENRENDER) {
			if (isVSyncChanged) {
				RecreateSwapChain();

				isVSyncChanged = false;
			}

			result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
			imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				RecreateImages();

				if (!isWindowMinimized) {
					frame = samplesPerFrame;
					samples = samplesPerFrame;
				} else {
					isWindowMinimized = false;
				}

				return;
			} else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
				throw std::runtime_error("Failed To Acquire Swap Chain Image!");
			}
		}

		if (isGraphicsRender) {
			vkResetFences(device, 1, &inFlightFences[currentFrame]);

			vkResetCommandBuffer(graphicsCommandBuffers[currentFrame], 0);
			RecordGraphicsCommandBuffer(graphicsCommandBuffers[currentFrame], imageIndex);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &graphicsCommandBuffers[currentFrame];

			std::vector<VkSemaphore> waitSemaphores;
			waitSemaphores.push_back(computeFinishedSemaphores[currentFrame]);
			if (OFFSCREENRENDER) {
				submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
				submitInfo.pWaitSemaphores = waitSemaphores.data();
			} else {
				waitSemaphores.push_back(imageAvailableSemaphores[currentFrame]);
				submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
				submitInfo.pWaitSemaphores = waitSemaphores.data();
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
			}

			if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
				throw std::runtime_error("Failed To Submit Draw Command Buffer!");
			}
		}

		if (!OFFSCREENRENDER) {
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
			VkSwapchainKHR swapChains[] = {swapChain};
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr; // Checking If Presentation Was Successful For Multiple Swap Chains

			result = vkQueuePresentKHR(presentQueue, &presentInfo);

			if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR) || isViewPortResized) {
				RecreateImages();

				if (!isWindowMinimized) {
					frame = 0;
					samples = 0;
				} else {
					isWindowMinimized = false;
				}

				isViewPortResized = false;
			} else if (result != VK_SUCCESS) {
				throw std::runtime_error("Failed To Present Swap Chain Image!");
			}
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
    
    void MainLoop() {
        bool isRunning = true;
		bool isWindowFocused = false;

		uint64_t start = 0;
		uint64_t end = 0;
		uint64_t prevEnd = 0;

		if (OFFSCREENRENDER) {
			samplesPerFrame = NUMSAMPLESPERFRAME;
			pathLength = PATHLENGTH;
			frame = samplesPerFrame;
			samples = samplesPerFrame;
			start = SDL_GetPerformanceCounter();
		}

		glm::vec2 cursorPos = glm::vec2(0.0f, 0.0f);
		glm::vec3 deltaCamPos = glm::vec3(0.0f, 0.0f, 0.0f);

		std::vector<float> frames;
		std::vector<float> spectra;
		std::vector<float> tonemapGraph;

		sphere newsphere = { { 0.0f, 0.0f, 0.0f }, 1.0f, 1 };
		plane newplane = { { 0.0f, 0.0f, 0.0f }, 1 };
		box newbox = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1 };
		lens newlens { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 1.0f, 1.0f, 0.0f, true, 1 };
		material newmaterial = { { 550.0f, 100.0f, 0 }, { 5500.0f, 0.0f } };

		world1(camera, spheres, planes, boxes, lenses, materials);

        while (isRunning) {
			if (!OFFSCREENRENDER) {
				SDLHandleEvents(isRunning, cursorPos, camera.angle, deltaCamPos, isWindowFocused);
				deltaCamPos *= 3.0f / FPS;
				UpdateCameraPos(camera.pos, glm::radians(camera.angle), deltaCamPos);

				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplSDL2_NewFrame();
				ImGui::NewFrame();

				ImGuiWindowFlags WinFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

				{
					ImGui::Begin("Scene", NULL, WinFlags);
					ImGui::SetWindowPos(ImVec2(W - ImGui::GetWindowWidth(), 0));
					ImGui::Text("Render Time: %0.3f ms (%0.1f FPS)", 1000.0f / FPS, FPS);
					ImGui::PlotLines("", frames.data(), (int)frames.size(), 0, NULL, 0.0f, 30.0f, ImVec2(303, 100));
					ImGui::Text("Resolution: (%i, %i) px", W, H);
					ImGui::Text("Samples: %i", samples);
					ImGui::Text("Camera Angle: (%0.3f, %0.3f)", camera.angle.x, camera.angle.y);
					ImGui::Text("Camera Pos: (%0.3f, %0.3f, %0.3f)", camera.pos.x, camera.pos.y, camera.pos.z);
					isVSyncChanged = ImGui::Checkbox("VSync", &VSync);
					isReset |= ImGui::DragInt("Samples/Frame", &samplesPerFrame, 0.02f, 1, 100);
					isReset |= ImGui::DragInt("Path Length", &pathLength, 0.02f);
					ImGui::Separator();

					if (ImGui::CollapsingHeader("Post Processing")) {
						if (ImGui::BeginTable("Tonemap Table", 1)) {
							ImGui::TableSetupColumn("Tonemap");
							ImGui::TableHeadersRow();
							ItemsTable("None", tonemap, 0, 1);
							ItemsTable("Reinhard", tonemap, 1, 1);
							ItemsTable("ACES Film", tonemap, 2, 1);
							ItemsTable("DEUCES", tonemap, 3, 1);
							ImGui::EndTable();
						}
						tonemapGraph.clear();
						for (int i = 1; i < 101; i++) {
							float x = 0.01f * (float)i;
							tonemapGraph.push_back(tonemapping(x, tonemap));
						}

						ImGui::PlotLines("", tonemapGraph.data(), (int)tonemapGraph.size(), 0, NULL, 0.0f, 1.0f, ImVec2(303, 100));
					}

					if (ImGui::CollapsingHeader("Camera")) {
						isReset |= ImGui::DragFloat("Persistence", &persistence, 0.00025f, 0.00025f, 1.0f, "%0.5f");
						isReset |= ImGui::DragInt("ISO", &camera.ISO, 50, 50, 819200);
						isReset |= ImGui::DragFloat("Camera Size", &camera.size, 0.001f, 0.001f, 5.0f, "%0.3f");
						isReset |= ImGui::DragFloat("Aperture Size", &camera.apertureSize, 0.0001f, 0.0001f, 10.0f, "%0.4f");
						isReset |= ImGui::DragFloat("Aperture Dist", &camera.apertureDist, 0.001f, 0.001f, camera.lensDistance, "%0.3f");
						float fov = 2.0f * glm::degrees(::atan(0.5f * camera.size / camera.apertureDist));
						ImGui::Text("FOV: %0.0f", fov);
						ImGui::Separator();

						ImGui::Text("Lens");
						isReset |= ImGui::DragFloat("Radius", &camera.lensRadius, 0.0005f, 0.0005f, 10.0f, "%0.4f");
						isReset |= ImGui::DragFloat("Focal Length", &camera.lensFocalLength, 0.0005f, 0.0005f, 10.0f, "%0.4f");
						isReset |= ImGui::DragFloat("Thickness", &camera.lensThickness, 0.0005f, 0.0f, 1.0f, "%0.4f");
						isReset |= ImGui::DragFloat("Distance", &camera.lensDistance, 0.001f, 0.001f, 100.0f, "%0.3f");
					}
					ImGui::Separator();

					if (ImGui::CollapsingHeader("Objects")) {
						static int objectsSelection = 0;

						if (ImGui::Button("Add New Sphere")) {
							spheres.push_back(newsphere);
						}
						if (ImGui::Button("Add New Plane")) {
							planes.push_back(newplane);
						}
						if (ImGui::Button("Add New Box")) {
							boxes.push_back(newbox);
						}
						if (ImGui::Button("Add New Lens")) {
							lenses.push_back(newlens);
						}
						ImGui::Separator();

						int numObjs = 0;
						if ((objectsSelection < (numObjs + spheres.size())) && (objectsSelection > (numObjs - 1))) {
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Sphere %i", objectsSelection - numObjs + 1);
							isUpdateUBO |= ImGui::DragFloat3("Position", spheres[objectsSelection - numObjs].pos, 0.01f);
							isUpdateUBO |= ImGui::DragFloat("Radius", &spheres[objectsSelection - numObjs].radius, 0.01f);
							isUpdateUBO |= ImGui::DragInt("Material ID", &spheres[objectsSelection - numObjs].materialID, 0.02f);
						}
						numObjs += (int)spheres.size();

						if ((objectsSelection < (numObjs + planes.size())) && (objectsSelection > (numObjs - 1))) {
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Plane %i", objectsSelection - numObjs + 1);
							isUpdateUBO |= ImGui::DragFloat3("Position", planes[objectsSelection - numObjs].pos, 0.01f);
							isUpdateUBO |= ImGui::DragInt("Material ID", &planes[objectsSelection - numObjs].materialID, 0.02f);
						}
						numObjs += (int)planes.size();

						if ((objectsSelection < (numObjs + boxes.size())) && (objectsSelection > (numObjs - 1))) {
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Box %i", objectsSelection - numObjs + 1);
							isUpdateUBO |= ImGui::DragFloat3("Position", boxes[objectsSelection - numObjs].pos, 0.01f);
							isUpdateUBO |= ImGui::DragFloat3("Rotation", boxes[objectsSelection - numObjs].rotation, 0.1f);
							isUpdateUBO |= ImGui::DragFloat3("Size", boxes[objectsSelection - numObjs].size, 0.01f);
							isUpdateUBO |= ImGui::DragInt("Material ID", &boxes[objectsSelection - numObjs].materialID, 0.02f);
						}
						numObjs += (int)boxes.size();

						if ((objectsSelection < (numObjs + lenses.size())) && (objectsSelection > (numObjs - 1))) {
							ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Lens %i", objectsSelection - numObjs + 1);
							isUpdateUBO |= ImGui::DragFloat3("Position", lenses[objectsSelection - numObjs].pos, 0.01f);
							isUpdateUBO |= ImGui::DragFloat3("Rotation", lenses[objectsSelection - numObjs].rotation, 0.1f);
							isUpdateUBO |= ImGui::DragFloat("Radius", &lenses[objectsSelection - numObjs].radius, 0.001f);
							isUpdateUBO |= ImGui::DragFloat("Focal Length", &lenses[objectsSelection - numObjs].focalLength, 0.001f);
							isUpdateUBO |= ImGui::DragFloat("Thickness", &lenses[objectsSelection - numObjs].thickness, 0.001f);
							isUpdateUBO |= ImGui::Checkbox("Convex Lens", &lenses[objectsSelection - numObjs].isConverging);
							isUpdateUBO |= ImGui::DragInt("Material ID", &lenses[objectsSelection - numObjs].materialID, 0.02f);
						}
						numObjs += (int)lenses.size();
						ImGui::Separator();

						if (ImGui::BeginTable("Objects Table", 1)) {
							ImGui::TableSetupColumn("Object");
							ImGui::TableHeadersRow();
							ItemsTable("Sphere %i", objectsSelection, 0, (int)spheres.size());
							ItemsTable("Plane %i", objectsSelection, (int)(spheres.size()), (int)planes.size());
							ItemsTable("Box %i", objectsSelection, (int)(spheres.size() + planes.size()), (int)boxes.size());
							ItemsTable("Lens %i", objectsSelection, (int)(spheres.size() + planes.size() + boxes.size()), (int)lenses.size());
							ImGui::EndTable();
						}
					}
					ImGui::Separator();

					if (ImGui::CollapsingHeader("Materials")) {
						static int materialsSelection = 0;

						if (ImGui::Button("Add New Material")) {
							materials.push_back(newmaterial);
							isUpdateUBO = true;
						}
						ImGui::Separator();

						ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Material %i", materialsSelection + 1);
						ImGui::Text("Reflection");
						ImGui::PushID("Reflection");
						spectra.clear();
						for (int i = 1; i < 101; i++) {
							float x = 0.01f * float(i) * 330.0f + 390.0f;
							spectra.push_back(SpectralPowerDistribution(x, materials[materialsSelection].reflection[0], 
							materials[materialsSelection].reflection[1], materials[materialsSelection].reflection[2]));
						}

						ImGui::PlotLines("", spectra.data(), (int)spectra.size(), 0, NULL, 0.0f, 1.0f, ImVec2(303, 100));
						isUpdateUBO |= ImGui::DragFloat("Peak Lambda", &materials[materialsSelection].reflection[0], 1.0f, 0.0f, 1200.0f);
						isUpdateUBO |= ImGui::DragFloat("Sigma", &materials[materialsSelection].reflection[1], 0.5f, 0.0f, 100.0f);
						bool isInvertBool;
						isInvertBool = (bool)materials[materialsSelection].reflection[2];
						isUpdateUBO |= ImGui::Checkbox("Invert", &isInvertBool);
						materials[materialsSelection].reflection[2] = (float)isInvertBool;
						ImGui::PopID();

						ImGui::Text("Emission");
						ImGui::PushID("Emission");
						spectra.clear();
						for (int i = 1; i < 101; i++) {
							float x = float(i) * 12e-9f;
							spectra.push_back(BlackBodyRadiation(x, materials[materialsSelection].emission[0]) / BlackBodyRadiationPeak(materials[materialsSelection].emission[0]));
						}

						ImGui::PlotLines("", spectra.data(), (int)spectra.size(), 0, NULL, 0.0f, 1.0f, ImVec2(303, 100));
						isUpdateUBO |= ImGui::DragFloat("Temperature", &materials[materialsSelection].emission[0], 5.0f);
						isUpdateUBO |= ImGui::DragFloat("Luminosity", &materials[materialsSelection].emission[1], 0.1f);
						ImGui::PopID();
						ImGui::Separator();

						if (ImGui::BeginTable("Materials Table", 1)) {
							ImGui::TableSetupColumn("Materials");
							ImGui::TableHeadersRow();
							ItemsTable("Material %i", materialsSelection, 0, (int)materials.size());
							ImGui::EndTable();
						}
					}

					isWindowFocused = ImGui::IsWindowFocused();
					ImGui::End();
				}

				ImGui::Render();

				isReset |= isUpdateUBO;
			}

			DrawFrame();

			if (OFFSCREENRENDER) {
				prevEnd = end;
				end = SDL_GetPerformanceCounter();
				double frequency = (double)SDL_GetPerformanceFrequency();
				double dtime = (double)(end - prevEnd) / frequency;
				double speed = (double)samplesPerFrame / dtime;
				double timeElapsed = (double)(end - start) / frequency;
				double progress = (double)samples / (double)NUMSAMPLES;
				int percentage = (int)(100.0 * progress);
				double timeRemaining = timeElapsed * ((1.0 / progress) - 1.0);

				std::string progressBar;
				for (int i = 0; i < percentage; i++) {
					progressBar.push_back((char)219);
				}
				for (int i = percentage; i < 100; i++) {
					progressBar.push_back((char)32);
				}

				printf("Rendering: %i%%|%s| %i/%i [%0.1fs|%0.1fs, %0.3fSPP/s] \r", percentage, progressBar.data(), samples, NUMSAMPLES, timeElapsed, timeRemaining, speed);
				if (samples >= NUMSAMPLES) {
					std::cout << std::endl;
					printf("Rendering Completed In %0.3fs. \n", timeElapsed);
					isRunning = false;
				}
			}

			frame += samplesPerFrame;
			if (isReset) {
				samples = samplesPerFrame;
				isReset = false;
			} else {
				samples += samplesPerFrame;
			}

			if (!OFFSCREENRENDER) {
				FPS = 1.0f / io->DeltaTime;
				if (frames.size() > 100) {
					for (size_t i = 1; i < frames.size(); i++) {
						frames[i - 1] = frames[i];
					}
					frames[frames.size() - 1] = 1000.0f / FPS;
				}
				else {
					frames.push_back(1000.0f / FPS);
				}
			}
        }

		vkDeviceWaitIdle(device);

		if (OFFSCREENRENDER) {
			VkImageSubresource subresource{};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			VkSubresourceLayout subresourceLayout{};
			vkGetImageSubresourceLayout(device, saveImage, &subresource, &subresourceLayout);

			unsigned char* pixels = new unsigned char[W * H * 4];
			vkMapMemory(device, saveImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&pixels);

			SDL_Surface* frameSurface = SDL_CreateRGBSurfaceFrom(pixels, W, H, 8 * 4, subresourceLayout.rowPitch, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
			IMG_SavePNG(frameSurface, "render.png");
			std::cout << "Rendered Image Has Been Successfully Saved." << std::endl;
			SDL_FreeSurface(frameSurface);

			vkUnmapMemory(device, saveImageMemory);
			delete[] pixels;
		}
    }

    void CleanUp() {
		CleanUpImages();
		CleanUpTexelBuffer();

		if (!OFFSCREENRENDER) {
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		if (!OFFSCREENRENDER) {
			vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyFence(device, computeInFlightFences[i], nullptr);
			vkDestroySemaphore(device, computeFinishedSemaphores[i], nullptr);

			vkDestroyFence(device, inFlightFences[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyPipeline(device, computePipeline, nullptr);
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
		vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);

		for (VkShaderModule shaderModule : shaderModules) {
			vkDestroyShaderModule(device, shaderModule, nullptr);
		}

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
    App app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}