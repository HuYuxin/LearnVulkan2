#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include <stb_image.h>
#include <tiny_obj_loader.h>

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

#include "Camera.hpp"
#include "Cube.hpp"
#include "Floor.hpp"
#include "utility.hpp"
#include "Vertex.hpp"
#include "VulkanInstance.hpp"

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

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
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
            transitionImageLayout(mVulkanInstance, swapChainImage, mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
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

    void recreateSwapChain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface)
    {
        int width = 0, height = 0;
        while(width == 0 || height == 0) {
            glfwGetFramebufferSize(mWindow, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(logicalDevice);

        cleanupSwapChain(logicalDevice);

        createSwapChain(physicalDevice, logicalDevice, surface);
        createSwapChainImageViews(logicalDevice);
        createMultisampleColorResources(logicalDevice);
        createMultisampleDepthResources(physicalDevice, logicalDevice);
        createShadowMapDepthResources(physicalDevice, logicalDevice);
        createShadowMapGraphicsPipeline(logicalDevice);
        createLambertGraphicsPipeline(logicalDevice);
    }

    void createShadowMapGraphicsPipeline(const VkDevice& logicalDevice)
    {
        auto shadowMapVertShaderCode = readFile("shaders/shadowMapVert.spv");
        auto shadowMapFragShaderCode = readFile("shaders/shadowMapFrag.spv");

        VkShaderModule shadowMapVertShaderModule = mVulkanInstance.createShaderModule(shadowMapVertShaderCode);
        VkShaderModule shadowMapFragShaderModule = mVulkanInstance.createShaderModule(shadowMapFragShaderCode);
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = shadowMapVertShaderModule;
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shadowMapFragShaderModule;
        fragShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo shadowMapShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDescription = Vertex::getBindingDescription();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        auto vertexAttributeDescription = Vertex::getAttributeDescriptions();
        vertexInputInfo.vertexAttributeDescriptionCount = 3;
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)mSwapChainExtent.width;
        viewport.height = (float)mSwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = mSwapChainExtent;
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mShadowMapDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &mShadowMapPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadowmap pipeline layout!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkFormat depthFormat = findDepthFormat(mVulkanInstance.getPhysicalDevice());
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shadowMapShaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr;
        pipelineInfo.layout = mShadowMapPipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mShadowMapGraphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(logicalDevice, shadowMapVertShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, shadowMapFragShaderModule, nullptr);
    }

    void createLambertDescriptorSetLayout(const VkDevice& logicalDevice) {
        std::vector<VkDescriptorSetLayoutBinding> lambertUniformBindings;
        lambertUniformBindings.resize(3);

        lambertUniformBindings[0].binding = 0;
        lambertUniformBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lambertUniformBindings[0].descriptorCount = 1;
        lambertUniformBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        lambertUniformBindings[0].pImmutableSamplers = nullptr;

        lambertUniformBindings[1].binding = 1;
        lambertUniformBindings[1].descriptorCount = 1;
        lambertUniformBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        lambertUniformBindings[1].pImmutableSamplers = nullptr;
        lambertUniformBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        lambertUniformBindings[2].binding = 2;
        lambertUniformBindings[2].descriptorCount = 1;
        lambertUniformBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        lambertUniformBindings[2].pImmutableSamplers = nullptr;
        lambertUniformBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(lambertUniformBindings.size());
        layoutInfo.pBindings = lambertUniformBindings.data();
        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mLambertDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create lambert descriptor set layout!");
        }
    }

    void createLambertGraphicsPipeline(const VkDevice& logicalDevice) {
        auto lambertVertShaderCode = readFile("shaders/lambertVert.spv");
        auto lambertFragShaderCode = readFile("shaders/lambertFrag.spv");

        VkShaderModule lambertVertShaderModule = mVulkanInstance.createShaderModule(lambertVertShaderCode);
        VkShaderModule lambertFragShaderModule = mVulkanInstance.createShaderModule(lambertFragShaderCode);
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = lambertVertShaderModule;
        vertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = lambertFragShaderModule;
        fragShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo cubeShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDescription = Vertex::getBindingDescription();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        auto vertexAttributeDescription = Vertex::getAttributeDescriptions();
        vertexInputInfo.vertexAttributeDescriptionCount = 3;
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)mSwapChainExtent.width;
        viewport.height = (float)mSwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = mSwapChainExtent;
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = mVulkanInstance.getMsaaSamples();
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mLambertDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &mLambertPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkFormat colorFormat = mSwapChainImageFormat;
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorFormat;
        VkFormat depthFormat = findDepthFormat(mVulkanInstance.getPhysicalDevice());
        renderingInfo.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = cubeShaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr;
        pipelineInfo.layout = mLambertPipelineLayout;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mLambertGraphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(logicalDevice, lambertFragShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, lambertVertShaderModule, nullptr);
    }

    void loadObjects() {
        Object* cube = new Cube();
        objects.push_back(cube);
        Object* floor = new Floor();
        objects.push_back(floor);
    }

    void initializeObjects() {
        for (Object* obj : objects) {
            obj->initializeObject();
            obj->createVertexBuffer(mVulkanInstance);
            obj->createIndexBuffer(mVulkanInstance);
            obj->createTextures(mVulkanInstance);
            obj->createDescriptorSets(mVulkanInstance, MAX_FRAMES_IN_FLIGHT, mDescriptorPool, mLambertDescriptorSetLayout, mUniformsBuffers, sizeof(UniformBufferObject), mShadowMapImageViews, mShadowMapSampler);
        }
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
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 4);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3);
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
        transitionImageLayout(mVulkanInstance, mDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
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
            transitionImageLayout(mVulkanInstance, mShadowMapImages[i], depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
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
        createShadowMapDescriptorSetLayout(mVulkanInstance.getLogicalDevice());
        createShadowMapDescriptorSets(mVulkanInstance.getLogicalDevice());
        createShadowMapGraphicsPipeline(mVulkanInstance.getLogicalDevice());
        createLambertDescriptorSetLayout(mVulkanInstance.getLogicalDevice());
        createLambertGraphicsPipeline(mVulkanInstance.getLogicalDevice());
        loadObjects();
        initializeObjects();
        createCommandBuffer(mVulkanInstance.getCommandPool(), mVulkanInstance.getLogicalDevice());
        createSyncObjects(mVulkanInstance.getLogicalDevice());
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
        ubo.lightPos = glm::vec3(5, 5, 5);

        float nearPlane = 0.1f;
        float farPlane = 25.0f;
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearPlane, farPlane);
        lightProjection[1][1] *= -1;
        glm::mat4 lightView = glm::lookAt(glm::vec3(10.0f, 5.0f, 10.0f),
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
        transitionImageLayout(mVulkanInstance, mShadowMapImages[imageIndex], depthFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
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
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowMapGraphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowMapPipelineLayout, 0, 1, &mShadowMapDescriptorSets[currentFrameIndex], 0, nullptr);
        for (Object* obj : objects) {
            VkBuffer vertexBuffers[] = {obj->getVertexBuffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, obj->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->getIndices().size()), 1, 0, 0, 0);
        }
        vkCmdEndRendering(commandBuffer);
        transitionImageLayout(mVulkanInstance, mShadowMapImages[imageIndex], depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        transitionImageLayout(mVulkanInstance, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
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
        for (Object* obj : objects) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLambertGraphicsPipeline);
            VkBuffer vertexBuffers[] = {obj->getVertexBuffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, obj->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLambertPipelineLayout, 0, 1, &(obj->getDescriptorSets()[currentFrameIndex]), 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->getIndices().size()), 1, 0, 0, 0);
        }
        vkCmdEndRendering(commandBuffer);
        transitionImageLayout(mVulkanInstance, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

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
        vkResetFences(logicalDevice, 1, &mInFlightFences[mCurrentFrame]);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain(physicalDevice, logicalDevice, surface);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(mCurrentFrame);
        vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);
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
            processInput();
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
            vkDestroyImage(logicalDevice, mShadowMapImages[i], nullptr);
            vkDestroyImageView(logicalDevice, mShadowMapImageViews[i], nullptr);
            vkFreeMemory(logicalDevice, mShadowMapImageMemory[i], nullptr);
            vkDestroyImageView(logicalDevice, mSwapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, mSwapChain, nullptr);
    }

    void processInput() {
        if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::FORWARD);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::BACKWARD);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::LEFT);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::RIGHT);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_R) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::UP);
        }
        if (glfwGetKey(mWindow, GLFW_KEY_F) == GLFW_PRESS) {
            mCamera->translate(CameraMovement::DOWN);
        }

        if (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(mWindow, &xpos, &ypos);
            if (!mIsLeftMouseButtonPressed) {
                mIsLeftMouseButtonPressed = true;
                mMousePos = glm::vec2(xpos, ypos);
                mMousePosPrev = mMousePos;
            } else {
                mMousePosPrev = mMousePos;
                mMousePos = glm::vec2(xpos, ypos);
                glm::vec2 delta = mMousePos - mMousePosPrev;
                if (delta.x != 0 || delta.y != 0) {
                    mCamera->rotateAroundCameraUpAxis(-delta.x * 0.1);
                    mCamera->rotateAroundCameraLeftAxis(delta.y * 0.1);
                }
            }
        } else {
            mIsLeftMouseButtonPressed = false;
        }
    }

    void cleanup()
    {
        VkDevice logicalDevice = mVulkanInstance.getLogicalDevice();
        cleanupSwapChain(logicalDevice);
        vkDestroySampler(logicalDevice, mShadowMapSampler, nullptr);
        for (Object* obj : objects) {
            obj->clearResource(mVulkanInstance);
        }
        vkDestroyPipeline(logicalDevice, mShadowMapGraphicsPipeline, nullptr);
        vkDestroyPipeline(logicalDevice, mLambertGraphicsPipeline, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroyBuffer(logicalDevice, mUniformsBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mUniformBuffersMemory[i], nullptr);
            vkDestroyBuffer(logicalDevice, mShadowMapUniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mShadowMapUniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mShadowMapDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(logicalDevice, mShadowMapPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mLambertDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(logicalDevice, mLambertPipelineLayout, nullptr);
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
    VkPipeline mShadowMapGraphicsPipeline;
    VkDescriptorSetLayout mLambertDescriptorSetLayout;
    VkPipelineLayout mLambertPipelineLayout;
    VkPipeline mLambertGraphicsPipeline;
    std::vector<VkCommandBuffer> mCommandBuffers;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishedSemaphores;
    std::vector<VkFence> mInFlightFences;
    uint32_t mCurrentFrame = 0;
    bool framebufferResized = false;
    std::vector<Object*> objects;
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
    glm::vec2 mMousePos;
    glm::vec2 mMousePosPrev;
    bool mIsLeftMouseButtonPressed = false;
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
