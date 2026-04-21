#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>



#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <vector>
#include <unordered_map>

#include "glTF3DModel.hpp"
#include "Camera.hpp"
#include "Cube.hpp"
#include "Floor.hpp"
#include "utility.hpp"
#include "Vertex.hpp"
#include "VulkanInstance.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        initDearImGui();
        mainLoop();
        cleanup();
    }

private:
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(mWindow, this);
        glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        // Required by VK_EXT_swapchain_maintenance1 device extension.
        extensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

        return extensions;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }


    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mVulkanInstance.getSurface(), &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(mWindow, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain(const VkPhysicalDevice& physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // If VkSurfaceCapabilitiesKHR.maxImageCount == 0, it means there is maximum limit
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = mVulkanInstance.getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = mVulkanInstance.findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(logicalDevice, mSwapChain, &imageCount, nullptr);
        mSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, mSwapChain, &imageCount, mSwapChainImages.data());
        mSwapChainImageFormat = surfaceFormat.format;
        mSwapChainExtent = extent;
        for (auto swapChainImage : mSwapChainImages) {
            transitionImageLayoutOnetimeSubmit(mVulkanInstance, swapChainImage, mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
        }
    }


    void createSwapChainImageViews(VkDevice localDevice)
    {
        mSwapChainImageViews.resize(mSwapChainImages.size());
        for (size_t i = 0; i < mSwapChainImages.size(); ++i)
        {
            mSwapChainImageViews[i] = createImageView(localDevice, mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void recreateSwapChain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface)
    {
        int width = 0, height = 0;
        while(width == 0 || height == 0) {
            glfwGetFramebufferSize(mWindow, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(logicalDevice);

        cleanupSwapChain(logicalDevice);
        cleanupShadowMap(logicalDevice);

        createSwapChain(physicalDevice, logicalDevice, surface);
        createSwapChainImageViews(logicalDevice);
        createMultisampleColorResources(logicalDevice);
        createMultisampleDepthResources(physicalDevice, logicalDevice);
        createShadowMapDepthResources(physicalDevice, logicalDevice);
        updateShadowMapDescriptorSets();
    }

    void createShadowMapDescriptorSetLayout(const VkDevice& logicalDevice) {
        VkDescriptorSetLayoutBinding shadowMapUboLayoutBinding{};
        shadowMapUboLayoutBinding.binding = 0;
        shadowMapUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowMapUboLayoutBinding.descriptorCount = 1;
        shadowMapUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        shadowMapUboLayoutBinding.pImmutableSamplers = nullptr;
        std::array<VkDescriptorSetLayoutBinding, 1> shadowMapBindings = {shadowMapUboLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t> (shadowMapBindings.size());
        layoutInfo.pBindings = shadowMapBindings.data();
        if(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mShadowMapDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow map descriptor set layout!");
        }
    }

    void createShadowMapGraphicsPipelineLayout(const VkDevice& logicalDevice) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mShadowMapDescriptorSetLayout;
        VkPushConstantRange pushConstantRange {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &mShadowMapPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadowmap pipeline layout!");
        }
    }

    void createShadowMapShaderObjects(const VkDevice& logicalDevice) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        VkShaderEXT shadowMapVertexShaderObject;
        size_t shadowMapVertexShaderCodeSize = 0;
        std::vector<char>  shadowMapVertexShaderCode = readFile("shaders/shadowMapVert.spv", shadowMapVertexShaderCodeSize);
        VkShaderCreateInfoEXT shadowMapVertexShaderCreateInfo = {};
        shadowMapVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
        shadowMapVertexShaderCreateInfo.flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
        shadowMapVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shadowMapVertexShaderCreateInfo.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shadowMapVertexShaderCreateInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
        shadowMapVertexShaderCreateInfo.pCode = shadowMapVertexShaderCode.data();
        shadowMapVertexShaderCreateInfo.codeSize = shadowMapVertexShaderCodeSize;
        shadowMapVertexShaderCreateInfo.pName = "main";
        shadowMapVertexShaderCreateInfo.setLayoutCount = 1;
        shadowMapVertexShaderCreateInfo.pSetLayouts = &mShadowMapDescriptorSetLayout;
        shadowMapVertexShaderCreateInfo.pushConstantRangeCount = 1;
        shadowMapVertexShaderCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreateShadersEXT(logicalDevice, 1, &shadowMapVertexShaderCreateInfo, nullptr, &shadowMapVertexShaderObject) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadowmap vertex shader object!");
        }
        VkShaderEXT shadowMapFragmentShaderObject;
        size_t shadowMapFragmentShaderCodeSize = 0;
        std::vector<char>  shadowMapFragmentShaderCode = readFile("shaders/shadowMapFrag.spv", shadowMapFragmentShaderCodeSize);
        VkShaderCreateInfoEXT shadowMapFragmentShaderCreateInfo = {};
        shadowMapFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
        shadowMapFragmentShaderCreateInfo.flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
        shadowMapFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shadowMapFragmentShaderCreateInfo.nextStage = 0;
        shadowMapFragmentShaderCreateInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
        shadowMapFragmentShaderCreateInfo.pCode = shadowMapFragmentShaderCode.data();
        shadowMapFragmentShaderCreateInfo.codeSize = shadowMapFragmentShaderCodeSize;
        shadowMapFragmentShaderCreateInfo.pName = "main";
        shadowMapFragmentShaderCreateInfo.setLayoutCount = 1;
        shadowMapFragmentShaderCreateInfo.pSetLayouts = &mShadowMapDescriptorSetLayout;
        shadowMapFragmentShaderCreateInfo.pushConstantRangeCount = 1;
        shadowMapFragmentShaderCreateInfo.pPushConstantRanges = &pushConstantRange;

        if(vkCreateShadersEXT(logicalDevice, 1, &shadowMapFragmentShaderCreateInfo, nullptr, &shadowMapFragmentShaderObject) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadowmap fragment shader object!");
        }
        mShadowMapShaderObjects[0] = std::move(shadowMapVertexShaderObject);
        mShadowMapShaderObjects[1] = std::move(shadowMapFragmentShaderObject);
    }

    void createLambertShaderObjects(const VkDevice& logicalDevice) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        VkShaderEXT lambertVertexShaderObject;
        size_t lambertVertexShaderCodeSize = 0;
        std::vector<char>  lambertVertexShaderCode = readFile("shaders/lambertVert.spv", lambertVertexShaderCodeSize);
        VkShaderCreateInfoEXT lambertVertexShaderCreateInfo = {};
        lambertVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
        lambertVertexShaderCreateInfo.flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
        lambertVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        lambertVertexShaderCreateInfo.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
        lambertVertexShaderCreateInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
        lambertVertexShaderCreateInfo.pCode = lambertVertexShaderCode.data();
        lambertVertexShaderCreateInfo.codeSize = lambertVertexShaderCodeSize;
        lambertVertexShaderCreateInfo.pName = "main";
        std::array<VkDescriptorSetLayout, 3> lambertSetLayouts = m3DModel->getDescriptorSetLayouts();
        lambertVertexShaderCreateInfo.setLayoutCount = lambertSetLayouts.size();
        lambertVertexShaderCreateInfo.pSetLayouts = lambertSetLayouts.data();
        lambertVertexShaderCreateInfo.pushConstantRangeCount = 1;
        lambertVertexShaderCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreateShadersEXT(logicalDevice, 1, &lambertVertexShaderCreateInfo, nullptr, &lambertVertexShaderObject) != VK_SUCCESS) {
            throw std::runtime_error("failed to create lambert vertex shader object!");
        }
        VkShaderEXT lambertFragmentShaderObject;
        size_t lambertFragmentShaderCodeSize = 0;
        std::vector<char>  lambertFragmentShaderCode = readFile("shaders/lambertFrag.spv", lambertFragmentShaderCodeSize);
        VkShaderCreateInfoEXT lambertFragmentShaderCreateInfo = {};
        lambertFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
        lambertFragmentShaderCreateInfo.flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
        lambertFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        lambertFragmentShaderCreateInfo.nextStage = 0;
        lambertFragmentShaderCreateInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
        lambertFragmentShaderCreateInfo.pCode = lambertFragmentShaderCode.data();
        lambertFragmentShaderCreateInfo.codeSize = lambertFragmentShaderCodeSize;
        lambertFragmentShaderCreateInfo.pName = "main";
        lambertFragmentShaderCreateInfo.setLayoutCount = 3;
        lambertFragmentShaderCreateInfo.pSetLayouts = lambertSetLayouts.data();
        lambertFragmentShaderCreateInfo.pushConstantRangeCount = 1;
        lambertFragmentShaderCreateInfo.pPushConstantRanges = &pushConstantRange;

        if(vkCreateShadersEXT(logicalDevice, 1, &lambertFragmentShaderCreateInfo, nullptr, &lambertFragmentShaderObject) != VK_SUCCESS) {
            throw std::runtime_error("failed to create lambert fragment shader object!");
        }
        mLambertShaderObjects[0] = std::move(lambertVertexShaderObject);
        mLambertShaderObjects[1] = std::move(lambertFragmentShaderObject);
    }

    void createLambertGraphicsPipelineLayout(const VkDevice& logicalDevice) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::array<VkDescriptorSetLayout, 3> lambertSetLayouts = m3DModel->getDescriptorSetLayouts();
        pipelineLayoutInfo.setLayoutCount = lambertSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = lambertSetLayouts.data();
        VkPushConstantRange pushConstantRange {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &mLambertPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void loadObjects() {
        m3DModel = new glTF3DModel(&mVulkanInstance);
    }

    void updateShadowMapDescriptorSets() {
        m3DModel->updateShadowMapDescriptorSets(MAX_FRAMES_IN_FLIGHT, mShadowMapImageViews, mShadowMapSampler);
    }

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 lightSpaceMatrix;
        glm::vec3 lightPos;
    };

    struct ShadowMapUniformBufferObject {
        glm::mat4 model;
        glm::mat4 lightSpaceMatrix;
    };

    void createUniformBuffers(const VkDevice& logicalDevice) {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        mUniformsBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        mUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        mUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            mVulkanInstance.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformsBuffers[i],
            mUniformBuffersMemory[i]);

            vkMapMemory(logicalDevice, mUniformBuffersMemory[i], 0, bufferSize, 0, &mUniformBuffersMapped[i]);
        }

        bufferSize = sizeof(ShadowMapUniformBufferObject);
        mShadowMapUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        mShadowMapUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        mShadowMapUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            mVulkanInstance.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mShadowMapUniformBuffers[i],
            mShadowMapUniformBuffersMemory[i]);

            vkMapMemory(logicalDevice, mShadowMapUniformBuffersMemory[i], 0, bufferSize, 0, &mShadowMapUniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool(const VkDevice& logicalDevice) {
        VkDescriptorPoolSize poolSize {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        if(vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createShadowMapDescriptorSets(const VkDevice& logicalDevice) {
        std::vector<VkDescriptorSetLayout> shadowMapLayouts(MAX_FRAMES_IN_FLIGHT, mShadowMapDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = shadowMapLayouts.data();
        mShadowMapDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if(vkAllocateDescriptorSets(logicalDevice, &allocInfo, mShadowMapDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate shadowmap descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mShadowMapUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ShadowMapUniformBufferObject);
            std::array<VkWriteDescriptorSet, 1> ShadowMapDescriptorWrites{};
            ShadowMapDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ShadowMapDescriptorWrites[0].dstSet = mShadowMapDescriptorSets[i];
            ShadowMapDescriptorWrites[0].dstBinding = 0;
            ShadowMapDescriptorWrites[0].dstArrayElement = 0;
            ShadowMapDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ShadowMapDescriptorWrites[0].descriptorCount = 1;
            ShadowMapDescriptorWrites[0].pBufferInfo = &bufferInfo;
            ShadowMapDescriptorWrites[0].pImageInfo = nullptr;
            ShadowMapDescriptorWrites[0].pTexelBufferView = nullptr;
            vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(ShadowMapDescriptorWrites.size()),
                ShadowMapDescriptorWrites.data(), 0, nullptr);
        }
    }

    void createCommandBuffer(const VkCommandPool& commandPool, const VkDevice& logicalDevice)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 2;
        mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }


    void createSyncObjects(const VkDevice& logicalDevice)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // initialize the fence in signaled state, so that the first vkWaitForFences() in drawFrame() won't stuck
        mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create sync objects");
            }
        }
    }

    void createShadowMapSampler(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0F;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &mShadowMapSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createCamera() {
        mCamera = new Camera(mSwapChainExtent.width, mSwapChainExtent.height);
    }

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }

            throw std::runtime_error("failed to find supported format!");
        }
    }

    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
        return findSupportedFormat(physicalDevice, 
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void createMultisampleDepthResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);
        createImage(mVulkanInstance, logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, mVulkanInstance.getMsaaSamples(), depthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);
        mDepthImageView = createImageView(logicalDevice, mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        transitionImageLayoutOnetimeSubmit(mVulkanInstance, mDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    }

    void createShadowMapDepthResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);
        mShadowMapImages.resize(mSwapChainImageViews.size());
        mShadowMapImageViews.resize(mSwapChainImageViews.size());
        mShadowMapImageMemory.resize(mSwapChainImageViews.size());
        for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
        {
            createImage(mVulkanInstance, logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mShadowMapImages[i],mShadowMapImageMemory[i]);
            mShadowMapImageViews[i] = createImageView(logicalDevice, mShadowMapImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            transitionImageLayoutOnetimeSubmit(mVulkanInstance, mShadowMapImages[i], depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        }
    }

    void createMultisampleColorResources(const VkDevice& logicalDevice) {
        VkFormat colorFormat = mSwapChainImageFormat;

        createImage(mVulkanInstance, logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, mVulkanInstance.getMsaaSamples(), colorFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mColorImage, mColorImageMemory);
        mColorImageView = createImageView(logicalDevice, mColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void initVulkan()
    {
        mVulkanInstance.initialize(enableValidationLayers, mWindow);
        createSwapChain(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice(), mVulkanInstance.getSurface());
        createSwapChainImageViews(mVulkanInstance.getLogicalDevice());
        createMultisampleColorResources(mVulkanInstance.getLogicalDevice());
        createMultisampleDepthResources(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createShadowMapDepthResources(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createCamera();
        createUniformBuffers(mVulkanInstance.getLogicalDevice());
        createShadowMapSampler(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createDescriptorPool(mVulkanInstance.getLogicalDevice());
        loadObjects();
        createShadowMapDescriptorSetLayout(mVulkanInstance.getLogicalDevice());
        createShadowMapDescriptorSets(mVulkanInstance.getLogicalDevice());
        createShadowMapShaderObjects(mVulkanInstance.getLogicalDevice());
        createShadowMapGraphicsPipelineLayout(mVulkanInstance.getLogicalDevice());
        createCommandBuffer(mVulkanInstance.getCommandPool(), mVulkanInstance.getLogicalDevice());
        createSyncObjects(mVulkanInstance.getLogicalDevice());
    }

    void initDearImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;
        ImGui_ImplGlfw_InitForVulkan(mWindow, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_4;
        init_info.Instance = mVulkanInstance.getInstance();
        init_info.PhysicalDevice = mVulkanInstance.getPhysicalDevice();
        init_info.Device = mVulkanInstance.getLogicalDevice();
        init_info.QueueFamily = mVulkanInstance.findQueueFamilies(mVulkanInstance.getPhysicalDevice()).graphicsFamily.value();
        init_info.Queue = mVulkanInstance.getGraphicsQueue();
        init_info.DescriptorPool = VK_NULL_HANDLE;
        init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(mSwapChainImages.size());
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.UseDynamicRendering = true;
        init_info.Allocator = nullptr;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {};
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.viewMask = 0;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = {&mSwapChainImageFormat};
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);
    }
        

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.view = glm::lookAt(mCamera->getCameraPosition(), mCamera->getCameraLookAtPosition(), mCamera->getCameraUp());
        ubo.proj = glm::perspective(mCamera->getFovY(), mCamera->getAspect(), mCamera->getNear(), mCamera->getFar());
        ubo.proj[1][1] *= -1;
        ubo.lightPos = glm::vec3(mLightPosX, mLightPosY, mLightPosZ);

        float nearPlane = 0.1f;
        float farPlane = 25.0f;
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearPlane, farPlane);
        lightProjection[1][1] *= -1;
        glm::mat4 lightView = glm::lookAt(glm::vec3(mLightPosX, mLightPosY, mLightPosZ),
                                  glm::vec3( 0.0f, 0.0f,  0.0f),
                                  glm::vec3( 0.0f, 1.0f,  0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        ubo.lightSpaceMatrix = lightSpaceMatrix;
        memcpy(mUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

        ShadowMapUniformBufferObject shadowMapUBO{};
        shadowMapUBO.model = ubo.model;
        shadowMapUBO.lightSpaceMatrix = lightSpaceMatrix;
        memcpy(mShadowMapUniformBuffersMapped[currentImage], &shadowMapUBO, sizeof(shadowMapUBO));
    }

    void recordCommandBuffer(VkPhysicalDevice physicalDevice, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrameIndex)
    {
        VkDeviceSize offsets[] = {0};

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to start recording command buffers");
        }

        VkFormat depthFormat = findDepthFormat(physicalDevice);
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = mSwapChainExtent;
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)mSwapChainExtent.width;
        viewport.height = (float)mSwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        const uint32_t sampleMask = 0xFF;
        const VkBool32 colorBlendEnables = false;
        const VkColorComponentFlags colorBlendComponentFlags = 0xf;
        std::array<VkShaderStageFlagBits, 2> shaderStageBits = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
        const Frustum cameraFrustum = mCamera->getFrustum();

        if (m3DModel->isInitialized()) {
            transitionImageLayout(mVulkanInstance, commandBuffer, mShadowMapImages[imageIndex], depthFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
            VkRenderingInfo shadowMapRenderInfo{};
            shadowMapRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            VkRect2D shadowMapRenderArea{};
            shadowMapRenderArea.offset = {0, 0};
            shadowMapRenderArea.extent = mSwapChainExtent;
            shadowMapRenderInfo.renderArea = shadowMapRenderArea;
            shadowMapRenderInfo.layerCount = 1;
            shadowMapRenderInfo.colorAttachmentCount = 0;
            VkRenderingAttachmentInfo shadowMapDepthAttachmentInfo{};
            shadowMapDepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            shadowMapDepthAttachmentInfo.imageView = mShadowMapImageViews[imageIndex];
            shadowMapDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            shadowMapDepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            shadowMapDepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            VkClearValue shadowMapClearValue{};
            shadowMapClearValue.depthStencil = {1.0f, 0};
            shadowMapDepthAttachmentInfo.clearValue = shadowMapClearValue;
            shadowMapRenderInfo.pDepthAttachment = &shadowMapDepthAttachmentInfo;
            vkCmdBeginRendering(commandBuffer, &shadowMapRenderInfo);
            vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);
	        vkCmdSetViewportWithCountEXT(commandBuffer, 1, &viewport);
	        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
		    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		    vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);
		    vkCmdSetDepthWriteEnable(commandBuffer, VK_TRUE);
		    vkCmdSetDepthCompareOp(commandBuffer, VK_COMPARE_OP_LESS);
		    vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		    vkCmdSetRasterizerDiscardEnable(commandBuffer, VK_FALSE);
		    vkCmdSetPolygonModeEXT(commandBuffer, VK_POLYGON_MODE_FILL);
		    vkCmdSetRasterizationSamplesEXT(commandBuffer, VK_SAMPLE_COUNT_1_BIT);
		    vkCmdSetAlphaToCoverageEnableEXT(commandBuffer, VK_FALSE);
		    vkCmdSetDepthBiasEnable(commandBuffer, VK_FALSE);
		    vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
		    vkCmdSetPrimitiveRestartEnable(commandBuffer, VK_FALSE);
		    vkCmdSetSampleMaskEXT(commandBuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);
		    vkCmdSetColorBlendEnableEXT(commandBuffer, 0, 1, &colorBlendEnables);
		    vkCmdSetColorWriteMaskEXT(commandBuffer, 0, 1, &colorBlendComponentFlags);
            VkVertexInputBindingDescription2EXT vertexInputBinding = m3DModel->getBindingDescription2EXT();
		    std::array<VkVertexInputAttributeDescription2EXT, 3> vertexAttributes = m3DModel->getAttributeDescriptions2EXT();
            vkCmdSetVertexInputEXT(commandBuffer, 1, &vertexInputBinding, 3, vertexAttributes.data());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowMapPipelineLayout, 0, 1, &mShadowMapDescriptorSets[currentFrameIndex], 0, nullptr);
            vkCmdBindShadersEXT(commandBuffer, 2, &shaderStageBits[0], &mShadowMapShaderObjects[0]);
            m3DModel->draw(commandBuffer, mShadowMapPipelineLayout, currentFrameIndex, nullptr, true);
            vkCmdEndRendering(commandBuffer);
            transitionImageLayout(mVulkanInstance, commandBuffer, mShadowMapImages[imageIndex], depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        }

        transitionImageLayout(mVulkanInstance, commandBuffer, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
        VkRenderingInfo lambertRenderInfo{};
        lambertRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        VkRect2D lambertRenderArea{};
        lambertRenderArea.offset = {0, 0};
        lambertRenderArea.extent = mSwapChainExtent;
        lambertRenderInfo.renderArea = lambertRenderArea;
        lambertRenderInfo.layerCount = 1;
        lambertRenderInfo.colorAttachmentCount = 1;
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.imageView = mColorImageView;
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        colorAttachmentInfo.resolveImageView = mSwapChainImageViews[imageIndex];
        colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkClearValue colorAttachmentClearValue{};
        colorAttachmentClearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        colorAttachmentInfo.clearValue = colorAttachmentClearValue;
        VkRenderingAttachmentInfo depthAttachmentInfo{};
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.imageView = mDepthImageView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkClearValue depthAttachmentClearValue{};
        depthAttachmentClearValue.depthStencil = {1.0f, 0};
        depthAttachmentInfo.clearValue = depthAttachmentClearValue;
        lambertRenderInfo.pColorAttachments = &colorAttachmentInfo;
        lambertRenderInfo.pDepthAttachment = &depthAttachmentInfo;
        vkCmdBeginRendering(commandBuffer, &lambertRenderInfo);
        if (m3DModel->isInitialized()) {
            vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);
	        vkCmdSetViewportWithCountEXT(commandBuffer, 1, &viewport);
	        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
		    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		    vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);
		    vkCmdSetDepthWriteEnable(commandBuffer, VK_TRUE);
		    vkCmdSetDepthCompareOp(commandBuffer, VK_COMPARE_OP_LESS);
		    vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		    vkCmdSetRasterizerDiscardEnable(commandBuffer, VK_FALSE);
		    vkCmdSetPolygonModeEXT(commandBuffer, VK_POLYGON_MODE_FILL);
		    vkCmdSetRasterizationSamplesEXT(commandBuffer, mVulkanInstance.getMsaaSamples());
		    vkCmdSetAlphaToCoverageEnableEXT(commandBuffer, VK_FALSE);
		    vkCmdSetDepthBiasEnable(commandBuffer, VK_FALSE);
		    vkCmdSetStencilTestEnable(commandBuffer, VK_FALSE);
		    vkCmdSetPrimitiveRestartEnable(commandBuffer, VK_FALSE);
		    vkCmdSetSampleMaskEXT(commandBuffer, mVulkanInstance.getMsaaSamples(), &sampleMask);
		    vkCmdSetColorBlendEnableEXT(commandBuffer, 0, 1, &colorBlendEnables);
		    vkCmdSetColorWriteMaskEXT(commandBuffer, 0, 1, &colorBlendComponentFlags);
            VkVertexInputBindingDescription2EXT vertexInputBinding = m3DModel->getBindingDescription2EXT();
		    std::array<VkVertexInputAttributeDescription2EXT, 3> vertexAttributes = m3DModel->getAttributeDescriptions2EXT();
		    vkCmdSetVertexInputEXT(commandBuffer, 1, &vertexInputBinding, 3, vertexAttributes.data());
            vkCmdBindShadersEXT(commandBuffer, 2, &shaderStageBits[0], &mLambertShaderObjects[0]);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLambertPipelineLayout, 0, 1, m3DModel->getUBODescriptorSet(currentFrameIndex), 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLambertPipelineLayout, 1, 1, m3DModel->getShadowMapDescriptorSet(currentFrameIndex), 0, nullptr);
            m3DModel->draw(commandBuffer, mLambertPipelineLayout, currentFrameIndex, &cameraFrustum, false);
        }
        vkCmdEndRendering(commandBuffer);

        // ImGui render pass: targets the already-resolved 1-sample swapchain image.
        // Must be a separate pass — ImGui's pipeline uses rasterizationSamples=1,
        // which would mismatch the MSAA (8-sample) attachments above.
        VkRenderingAttachmentInfo imguiColorAttachment{};
        imguiColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        imguiColorAttachment.imageView = mSwapChainImageViews[imageIndex];
        imguiColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imguiColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        imguiColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo imguiRenderInfo{};
        imguiRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        imguiRenderInfo.renderArea = {{0, 0}, mSwapChainExtent};
        imguiRenderInfo.layerCount = 1;
        imguiRenderInfo.colorAttachmentCount = 1;
        imguiRenderInfo.pColorAttachments = &imguiColorAttachment;
        // No depth attachment — ImGui is 2D UI and doesn't write depth.

        vkCmdBeginRendering(commandBuffer, &imguiRenderInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        vkCmdEndRendering(commandBuffer);

        transitionImageLayout(mVulkanInstance, commandBuffer, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame()
    {
        VkDevice logicalDevice = mVulkanInstance.getLogicalDevice();
        VkPhysicalDevice physicalDevice = mVulkanInstance.getPhysicalDevice();
        VkSurfaceKHR surface = mVulkanInstance.getSurface();
        vkWaitForFences(logicalDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain(physicalDevice, logicalDevice, surface);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Reset the fence only after confirming we will submit work that signals it.
        // If we reset before the early return above, the fence is permanently unsignaled
        // and the next vkWaitForFences call hangs forever.
        vkResetFences(logicalDevice, 1, &mInFlightFences[mCurrentFrame]);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // --- FPS calculation ---
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - mLastFrameTime).count();
        mLastFrameTime = now;
        mFrameCount++;
        mFpsAccumulator += deltaTime;
        if (mFpsAccumulator >= 0.5f) {
            mDisplayFps = mFrameCount / mFpsAccumulator;
            mFrameCount = 0;
            mFpsAccumulator = 0.0f;
        }

        // Add your UI elements HERE:
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
        ImGui::Begin("My Window");
        ImGui::Text("FPS: %.1f  (%.3f ms/frame)", mDisplayFps, (mDisplayFps > 0.0f) ? 1000.0f / mDisplayFps : 0.0f);
        ImGui::Text("Primitive Count: %d", m3DModel->getPrimitiveCount());
        ImGui::Separator();
        ImGui::InputText("Model Path", mModelPathBuf, sizeof(mModelPathBuf));
        if (ImGui::Button("Load Model")) {
            mSelectedModelPath = std::string(mModelPathBuf);
        }
        ImGui::Separator();
        ImGui::InputFloat("Light Position X", &mLightPosX, 0.1f, 1.0f, "%.1f");
        ImGui::InputFloat("Light Position Y", &mLightPosY, 0.1f, 1.0f, "%.1f");
        ImGui::InputFloat("Light Position Z", &mLightPosZ, 0.1f, 1.0f, "%.1f");
        ImGui::SliderFloat("Camera Yaw (Y-axis)", &mCameraYaw, -180.0f, 180.0f, "%.1f deg");
        ImGui::SliderFloat("Camera Pitch (X-axis)", &mCameraPitch, -89.0f, 89.0f, "%.1f deg");
        ImGui::End();

        // Hot-swap 3D model if user selected a new path
        if (!mSelectedModelPath.empty() && mSelectedModelPath != mCurrentModelPath) {
            vkDeviceWaitIdle(logicalDevice);

            // Destroy Lambert shader objects & pipeline layout (they reference m3DModel's descriptor set layouts)
            if (m3DModel->isInitialized()) {
                for (VkShaderEXT& shader : mLambertShaderObjects) {
                    vkDestroyShaderEXT(logicalDevice, shader, nullptr);
                }
                vkDestroyPipelineLayout(logicalDevice, mLambertPipelineLayout, nullptr);
                m3DModel->clearResource();
            }

            // Load new model (full init sequence)
            m3DModel->initialize(mSelectedModelPath);
            m3DModel->createVertexBuffer();
            m3DModel->createIndexBuffer();
            m3DModel->setupDescriptors(MAX_FRAMES_IN_FLIGHT, mUniformsBuffers,
                                       sizeof(UniformBufferObject),
                                       mShadowMapImageViews, mShadowMapSampler);

            // Recreate Lambert pipeline objects with new descriptor layouts
            createLambertShaderObjects(logicalDevice);
            createLambertGraphicsPipelineLayout(logicalDevice);

            mCurrentModelPath = mSelectedModelPath;
        }

        processInput();

        updateUniformBuffer(mCurrentFrame);
        vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);

        //ImGui::ShowDemoWindow();
        ImGui::Render();

        recordCommandBuffer(mVulkanInstance.getPhysicalDevice(), mCommandBuffers[mCurrentFrame], imageIndex, mCurrentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrame];

        VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(mVulkanInstance.getGraphicsQueue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkSwapchainPresentFenceInfoEXT presentFenceInfo = {};
        presentFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
        presentFenceInfo.pNext = nullptr;
        presentFenceInfo.swapchainCount = 1;
        presentFenceInfo.pFences = &mInFlightFences[mCurrentFrame];

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {mSwapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;
        presentInfo.pNext = &presentFenceInfo;
        result = vkQueuePresentKHR(mVulkanInstance.getPresentationQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain(physicalDevice, logicalDevice, surface);
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image");
        }
        mCurrentFrame = (mCurrentFrame+1) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(mWindow))
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(mVulkanInstance.getLogicalDevice());
    }

    void cleanupCamera()
    {
        delete mCamera;
    }

    void cleanupSwapChain(VkDevice logicalDevice)
    {
        vkDestroyImageView(logicalDevice, mColorImageView, nullptr);
        vkDestroyImage(logicalDevice, mColorImage, nullptr);
        vkFreeMemory(logicalDevice, mColorImageMemory, nullptr);
        vkDestroyImageView(logicalDevice, mDepthImageView, nullptr);
        vkDestroyImage(logicalDevice, mDepthImage, nullptr);
        vkFreeMemory(logicalDevice, mDepthImageMemory, nullptr);

        for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
        {
            vkDestroyImageView(logicalDevice, mSwapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, mSwapChain, nullptr);
    }

    void cleanupShadowMap(VkDevice logicalDevice)
    {
        for (size_t i = 0; i < mShadowMapImages.size(); ++i)
        {
            vkDestroyImage(logicalDevice, mShadowMapImages[i], nullptr);
            vkDestroyImageView(logicalDevice, mShadowMapImageViews[i], nullptr);
            vkFreeMemory(logicalDevice, mShadowMapImageMemory[i], nullptr);
        }
    }

    void processInput() {
        if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::FORWARD, 0.01f);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::BACKWARD, 0.01f);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::LEFT, 0.01f);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::RIGHT, 0.01f);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_R) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::UP, 0.01f);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_F) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::DOWN, 0.01f);
        }

        float deltaYaw = mCameraYaw - mCameraYawPrev;
        float deltaPitch = mCameraPitch - mCameraPitchPrev;

        if (deltaYaw != 0.0f) {
            mCamera->orbitAroundObjectUpAxis(deltaYaw, 0.0f, 0.0f);
            mCameraYawPrev = mCameraYaw;
        }
        if (deltaPitch != 0.0f) {
            mCamera->orbitAroundObjectHorizontalAxis(deltaPitch, 0.0f, 0.0f, 0.0f);
            mCameraPitchPrev = mCameraPitch;
        }
    }

    void cleanup()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        VkDevice logicalDevice = mVulkanInstance.getLogicalDevice();
        cleanupSwapChain(logicalDevice);
        cleanupShadowMap(logicalDevice);
        vkDestroySampler(logicalDevice, mShadowMapSampler, nullptr);
        if (m3DModel->isInitialized()) {
            m3DModel->clearResource();
        }
        delete m3DModel;
        for (VkShaderEXT& shadowMapShaderObject : mShadowMapShaderObjects) {
            vkDestroyShaderEXT(logicalDevice, shadowMapShaderObject, nullptr);
        }
        if (!mCurrentModelPath.empty()) {
            for (VkShaderEXT& lambertShaderObject : mLambertShaderObjects) {
                vkDestroyShaderEXT(logicalDevice, lambertShaderObject, nullptr);
            }
            vkDestroyPipelineLayout(logicalDevice, mLambertPipelineLayout, nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroyBuffer(logicalDevice, mUniformsBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mUniformBuffersMemory[i], nullptr);
            vkDestroyBuffer(logicalDevice, mShadowMapUniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mShadowMapUniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mShadowMapDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(logicalDevice, mShadowMapPipelineLayout, nullptr);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(logicalDevice, mImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, mRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, mInFlightFences[i], nullptr);
        }

        mVulkanInstance.destroy();
        glfwDestroyWindow(mWindow);
        glfwTerminate();
        cleanupCamera();
    }

    VulkanInstance mVulkanInstance;
    GLFWwindow *mWindow;
    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;
    std::vector<VkImage> mShadowMapImages;
    std::vector<VkImageView> mShadowMapImageViews;
    std::vector<VkDeviceMemory> mShadowMapImageMemory;
    VkSampler mShadowMapSampler;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    VkDescriptorSetLayout mShadowMapDescriptorSetLayout;
    VkPipelineLayout mShadowMapPipelineLayout;
    std::array<VkShaderEXT, 2> mShadowMapShaderObjects;
    //VkDescriptorSetLayout mLambertDescriptorSetLayout;
    std::array<VkShaderEXT, 2> mLambertShaderObjects;
    VkPipelineLayout mLambertPipelineLayout;
    std::vector<VkCommandBuffer> mCommandBuffers;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishedSemaphores;
    std::vector<VkFence> mInFlightFences;
    uint32_t mCurrentFrame = 0;
    bool framebufferResized = false;
    glTF3DModel* m3DModel;
    //std::vector<Object*> objects;
    VkImage mDepthImage;
    VkDeviceMemory mDepthImageMemory;
    VkImageView mDepthImageView;
    VkImage mColorImage;
    VkDeviceMemory mColorImageMemory;
    VkImageView mColorImageView;
    std::vector<VkBuffer> mUniformsBuffers;
    std::vector<VkDeviceMemory> mUniformBuffersMemory;
    std::vector<void*> mUniformBuffersMapped;
    std::vector<VkBuffer> mShadowMapUniformBuffers;
    std::vector<VkDeviceMemory> mShadowMapUniformBuffersMemory;
    std::vector<void*> mShadowMapUniformBuffersMapped;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mShadowMapDescriptorSets;
    Camera* mCamera;
    float mCameraYaw = 0.0f;
    float mCameraYawPrev = 0.0f;
    float mCameraPitch = 0.0f;
    float mCameraPitchPrev = 0.0f;
    float mLightPosX = 5.0f;
    float mLightPosY = 5.0f;
    float mLightPosZ = 10.0f;
    char mModelPathBuf[512] = "";
    std::string mSelectedModelPath;
    std::string mCurrentModelPath;

    // FPS tracking
    std::chrono::steady_clock::time_point mLastFrameTime = std::chrono::steady_clock::now();
    int   mFrameCount    = 0;
    float mFpsAccumulator = 0.0f;
    float mDisplayFps     = 0.0f;
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
