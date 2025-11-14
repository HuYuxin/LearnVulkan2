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

#include "camera.hpp"
#include "cube.hpp"
#include "floor.hpp"
#include "Vertex.hpp"
#include "vulkanInstance.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string CUBE_TEXTURE_PATH = "textures/Texturelabs_Wood_134M.jpg";
const std::string FLOOR_TEXTURE_PATH = "textures/floor.jpg";

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
    }

    // Helper function to create imageview
    VkImageView createImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }


    void createImageViews(VkDevice localDevice)
    {
        mSwapChainImageViews.resize(mSwapChainImages.size());
        for (size_t i = 0; i < mSwapChainImages.size(); ++i)
        {
            mSwapChainImageViews[i] = createImageView(localDevice, mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void createShadowMapRenderPass(VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pColorAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency1{};
        dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency1.dstSubpass = 0;
        dependency1.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependency1.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dependency1.dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency1.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency2.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependency2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        std::array<VkSubpassDependency, 2> dependencies = {dependency1, dependency2};

        std::array<VkAttachmentDescription, 1> attachments = {depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &mShadowMapRenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadow map render pass!");
        }

    }

    void createRenderPass(VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = mSwapChainImageFormat;
        colorAttachment.samples = mVulkanInstance.getMsaaSamples();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = mSwapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = mVulkanInstance.getMsaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency1{};
        dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency1.dstSubpass = 0;
        //dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency1.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        //dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency1.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency1.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        //dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        dependency1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependency2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        std::array<VkSubpassDependency, 2> dependencies = {dependency1, dependency2};

        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout(const VkDevice& logicalDevice) {
        VkDescriptorSetLayoutBinding cubeUboLayoutBinding{};
        cubeUboLayoutBinding.binding = 0;
        cubeUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cubeUboLayoutBinding.descriptorCount = 1;
        cubeUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        cubeUboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding cubeSamplerLayoutBinding{};
        cubeSamplerLayoutBinding.binding = 1;
        cubeSamplerLayoutBinding.descriptorCount = 1;
        cubeSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubeSamplerLayoutBinding.pImmutableSamplers = nullptr;
        cubeSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding shadowMapSamplerLayoutBinding{};
        shadowMapSamplerLayoutBinding.binding = 2;
        shadowMapSamplerLayoutBinding.descriptorCount = 1;
        shadowMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapSamplerLayoutBinding.pImmutableSamplers = nullptr;
        shadowMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> cubeBindings = {cubeUboLayoutBinding, cubeSamplerLayoutBinding, shadowMapSamplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(cubeBindings.size());
        layoutInfo.pBindings = cubeBindings.data();
        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mCubeDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create cube descriptor set layout!");
        }

        VkDescriptorSetLayoutBinding floorUboLayoutBinding{};
        floorUboLayoutBinding.binding = 0;
        floorUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        floorUboLayoutBinding.descriptorCount = 1;
        floorUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        floorUboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding floorSamplerLayoutBinding{};
        floorSamplerLayoutBinding.binding = 1;
        floorSamplerLayoutBinding.descriptorCount = 1;
        floorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        floorSamplerLayoutBinding.pImmutableSamplers = nullptr;
        floorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> floorBindings = {floorUboLayoutBinding, floorSamplerLayoutBinding, shadowMapSamplerLayoutBinding};
        layoutInfo.bindingCount = static_cast<uint32_t>(floorBindings.size());
        layoutInfo.pBindings = floorBindings.data();
        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &mFloorDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create floor descriptor set layout!");
        }


        VkDescriptorSetLayoutBinding shadowMapUboLayoutBinding{};
        shadowMapUboLayoutBinding.binding = 0;
        shadowMapUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowMapUboLayoutBinding.descriptorCount = 1;
        shadowMapUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        shadowMapUboLayoutBinding.pImmutableSamplers = nullptr;
        std::array<VkDescriptorSetLayoutBinding, 1> shadowMapBindings = {shadowMapUboLayoutBinding};
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
        createImageViews(logicalDevice);
        createColorResources(logicalDevice);
        createDepthResources(physicalDevice, logicalDevice);
        createFramebuffers(logicalDevice);
        mCube.createGraphicsPipeline(mVulkanInstance, mSwapChainExtent, mRenderPass, &mCubeDescriptorSetLayout);
        mFloor.createGraphicsPipeline(mVulkanInstance, mSwapChainExtent, mRenderPass, &mFloorDescriptorSetLayout);
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
        pipelineInfo.renderPass = mShadowMapRenderPass;
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

    void createFramebuffers(const VkDevice& logicalDevice)
    {
        mSwapChainFramebuffers.resize(mSwapChainImageViews.size());
        mShadowMapFramebuffers.resize(mSwapChainImageViews.size());

        for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
        {
            std::array<VkImageView, 3> attachments = {
                mColorImageView,
                mDepthImageView,
                mSwapChainImageViews[i],
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = mRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = mSwapChainExtent.width;
            framebufferInfo.height = mSwapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }

            std::array<VkImageView, 1> shadowMapFramebufferAttachments = {
                mShadowMapImageViews[i],
            };

            VkFramebufferCreateInfo shadowMapFramebufferInfo{};
            shadowMapFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            shadowMapFramebufferInfo.renderPass = mShadowMapRenderPass;
            shadowMapFramebufferInfo.attachmentCount = static_cast<uint32_t>(shadowMapFramebufferAttachments.size());
            shadowMapFramebufferInfo.pAttachments = shadowMapFramebufferAttachments.data();
            shadowMapFramebufferInfo.width = mSwapChainExtent.width;
            shadowMapFramebufferInfo.height = mSwapChainExtent.height;
            shadowMapFramebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &shadowMapFramebufferInfo, nullptr, &mShadowMapFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create shadow map framebuffer");
            }

        }
    }

    void loadCube() {
        mCube.createCubeVertexBuffer(mVulkanInstance);
        mCube.createCubeIndexBuffer(mVulkanInstance);
    }

    void loadFloor()
    {
        mFloor.createFloorVertexBuffer(mVulkanInstance);
        mFloor.createFloorIndexBuffer(mVulkanInstance);
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

    void createImage(const VkDevice& logicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = mVulkanInstance.findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(logicalDevice, image, imageMemory, 0);
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

    void createDescriptorSets(const VkDevice& logicalDevice) {
        std::vector<VkDescriptorSetLayout> cubeLayouts(MAX_FRAMES_IN_FLIGHT, mCubeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = cubeLayouts.data();
        mCubeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if(vkAllocateDescriptorSets(logicalDevice, &allocInfo, mCubeDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate cube descriptor sets!");
        }

        std::vector<VkDescriptorSetLayout> floorLayouts(MAX_FRAMES_IN_FLIGHT, mFloorDescriptorSetLayout);
        allocInfo.pSetLayouts = floorLayouts.data();
        mFloorDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if(vkAllocateDescriptorSets(logicalDevice, &allocInfo, mFloorDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate floor descriptor sets!");
        }

        std::vector<VkDescriptorSetLayout> shadowMapLayouts(MAX_FRAMES_IN_FLIGHT, mShadowMapDescriptorSetLayout);
        allocInfo.pSetLayouts = shadowMapLayouts.data();
        mShadowMapDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if(vkAllocateDescriptorSets(logicalDevice, &allocInfo, mShadowMapDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate shadowmap descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mUniformsBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo cubeImageInfo{};
            cubeImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            cubeImageInfo.imageView = mCubeTextureImageView;
            cubeImageInfo.sampler = mCubeTextureSampler;

            VkDescriptorImageInfo floorImageInfo{};
            floorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            floorImageInfo.imageView = mFloorTextureImageView;
            floorImageInfo.sampler = mFloorTextureSampler;

            VkDescriptorImageInfo shadowMapInfo{};
            shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowMapInfo.imageView = mShadowMapImageViews[i];
            shadowMapInfo.sampler = mShadowMapSampler;

            std::array<VkWriteDescriptorSet, 3> cubeDescriptorWrites{};
            cubeDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            cubeDescriptorWrites[0].dstSet = mCubeDescriptorSets[i];
            cubeDescriptorWrites[0].dstBinding = 0;
            cubeDescriptorWrites[0].dstArrayElement = 0;
            cubeDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cubeDescriptorWrites[0].descriptorCount = 1;
            cubeDescriptorWrites[0].pBufferInfo = &bufferInfo;
            cubeDescriptorWrites[0].pImageInfo = nullptr; // Optional
            cubeDescriptorWrites[0].pTexelBufferView = nullptr; // Optional

            cubeDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            cubeDescriptorWrites[1].dstSet = mCubeDescriptorSets[i];
            cubeDescriptorWrites[1].dstBinding = 1;
            cubeDescriptorWrites[1].dstArrayElement = 0;
            cubeDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            cubeDescriptorWrites[1].descriptorCount = 1;
            cubeDescriptorWrites[1].pImageInfo = &cubeImageInfo;

            cubeDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            cubeDescriptorWrites[2].dstSet = mCubeDescriptorSets[i];
            cubeDescriptorWrites[2].dstBinding = 2;
            cubeDescriptorWrites[2].dstArrayElement = 0;
            cubeDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            cubeDescriptorWrites[2].descriptorCount = 1;
            cubeDescriptorWrites[2].pImageInfo = &shadowMapInfo;

            vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(cubeDescriptorWrites.size()), 
                cubeDescriptorWrites.data(), 0, nullptr);
            
            std::array<VkWriteDescriptorSet, 3> FloorDescriptorWrites{};
            FloorDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            FloorDescriptorWrites[0].dstSet = mFloorDescriptorSets[i];
            FloorDescriptorWrites[0].dstBinding = 0;
            FloorDescriptorWrites[0].dstArrayElement = 0;
            FloorDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            FloorDescriptorWrites[0].descriptorCount = 1;
            FloorDescriptorWrites[0].pBufferInfo = &bufferInfo;
            FloorDescriptorWrites[0].pImageInfo = nullptr; // Optional
            FloorDescriptorWrites[0].pTexelBufferView = nullptr; // Optional

            FloorDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            FloorDescriptorWrites[1].dstSet = mFloorDescriptorSets[i];
            FloorDescriptorWrites[1].dstBinding = 1;
            FloorDescriptorWrites[1].dstArrayElement = 0;
            FloorDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            FloorDescriptorWrites[1].descriptorCount = 1;
            FloorDescriptorWrites[1].pImageInfo = &floorImageInfo;

            FloorDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            FloorDescriptorWrites[2].dstSet = mFloorDescriptorSets[i];
            FloorDescriptorWrites[2].dstBinding = 2;
            FloorDescriptorWrites[2].dstArrayElement = 0;
            FloorDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            FloorDescriptorWrites[2].descriptorCount = 1;
            FloorDescriptorWrites[2].pImageInfo = &shadowMapInfo; // Optional
            FloorDescriptorWrites[2].pTexelBufferView = nullptr; // Optional
            vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(FloorDescriptorWrites.size()), 
                FloorDescriptorWrites.data(), 0, nullptr);

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

    void transitionImageLayout(VulkanInstance& vulkanInstance, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = vulkanInstance.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        //if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        if (format >= VK_FORMAT_D16_UNORM && format <= VK_FORMAT_D32_SFLOAT_S8_UINT) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        vulkanInstance.endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VulkanInstance& instance, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        const VkCommandBuffer& commandBuffer = instance.beginSingleTimeCommands();
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        instance.endSingleTimeCommands(commandBuffer);
    }

    void generateMipmaps(VulkanInstance& instance, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(instance.getPhysicalDevice(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }
        
        const VkCommandBuffer& commandBuffer = instance.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;
        for(uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i-1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(commandBuffer,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                0, nullptr,
                                0, nullptr,
                                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i-1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(commandBuffer,
                            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1, &blit, VK_FILTER_LINEAR);
            
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels -1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        

        instance.endSingleTimeCommands(commandBuffer);
    }

    void createCubeTextureImage(const VkDevice& logicalDevice) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(CUBE_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        mMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        mVulkanInstance.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        stbi_image_free(pixels);

        createImage(logicalDevice, texWidth, texHeight, mMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mCubeTextureImage, mCubeTextureImageMemory);

        transitionImageLayout(mVulkanInstance, mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels);
        copyBufferToImage(mVulkanInstance, stagingBuffer, mCubeTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        generateMipmaps(mVulkanInstance, mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mMipLevels);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    void createFloorTextureImage(const VkDevice& logicalDevice) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(FLOOR_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        mMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        mVulkanInstance.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        stbi_image_free(pixels);

        createImage(logicalDevice, texWidth, texHeight, mMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mFloorTextureImage, mFloorTextureImageMemory);

        transitionImageLayout(mVulkanInstance, mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels);
        copyBufferToImage(mVulkanInstance, stagingBuffer, mFloorTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        generateMipmaps(mVulkanInstance, mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mMipLevels);
             
        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    void createCubeTextureImageView(VkDevice logicalDevice) {
        mCubeTextureImageView = createImageView(logicalDevice, mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels);
    }

    void createFloorTextureImageView(VkDevice logicalDevice) {
        mFloorTextureImageView = createImageView(logicalDevice, mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels);
    }

    void createCubeTextureSampler(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice) {
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
        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &mCubeTextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createFloorTextureSampler(const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice) {
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
        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &mFloorTextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
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

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);
        createImage(logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, mVulkanInstance.getMsaaSamples(), depthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);
        mDepthImageView = createImageView(logicalDevice, mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        transitionImageLayout(mVulkanInstance, mDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        mShadowMapImages.resize(mSwapChainImageViews.size());
        mShadowMapImageViews.resize(mSwapChainImageViews.size());
        mShadowMapImageMemory.resize(mSwapChainImageViews.size());
        for (size_t i = 0; i < mSwapChainImageViews.size(); ++i)
        {
            createImage(logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mShadowMapImages[i],mShadowMapImageMemory[i]);
            mShadowMapImageViews[i] = createImageView(logicalDevice, mShadowMapImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            transitionImageLayout(mVulkanInstance, mShadowMapImages[i], depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        }
    }

    void createColorResources(const VkDevice& logicalDevice) {
        VkFormat colorFormat = mSwapChainImageFormat;

        createImage(logicalDevice, mSwapChainExtent.width, mSwapChainExtent.height, 1, mVulkanInstance.getMsaaSamples(), colorFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mColorImage, mColorImageMemory);
        mColorImageView = createImageView(logicalDevice, mColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void initVulkan()
    {
        mVulkanInstance.initialize(enableValidationLayers, mWindow);
        createSwapChain(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice(), mVulkanInstance.getSurface());
        createImageViews(mVulkanInstance.getLogicalDevice());
        createShadowMapRenderPass(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createRenderPass(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createDescriptorSetLayout(mVulkanInstance.getLogicalDevice());
        createShadowMapGraphicsPipeline(mVulkanInstance.getLogicalDevice());
        createColorResources(mVulkanInstance.getLogicalDevice());
        createDepthResources(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createFramebuffers(mVulkanInstance.getLogicalDevice());
        createCubeTextureImage(mVulkanInstance.getLogicalDevice());
        createCubeTextureImageView(mVulkanInstance.getLogicalDevice());
        createCubeTextureSampler(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createFloorTextureImage(mVulkanInstance.getLogicalDevice());
        createFloorTextureImageView(mVulkanInstance.getLogicalDevice());
        createFloorTextureSampler(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        createShadowMapSampler(mVulkanInstance.getPhysicalDevice(), mVulkanInstance.getLogicalDevice());
        loadCube();
        loadFloor();
        mCube.createGraphicsPipeline(mVulkanInstance, mSwapChainExtent, mRenderPass, &mCubeDescriptorSetLayout);
        mFloor.createGraphicsPipeline(mVulkanInstance, mSwapChainExtent, mRenderPass, &mFloorDescriptorSetLayout);
        createUniformBuffers(mVulkanInstance.getLogicalDevice());
        createDescriptorPool(mVulkanInstance.getLogicalDevice());
        createDescriptorSets(mVulkanInstance.getLogicalDevice());
        createCommandBuffer(mVulkanInstance.getCommandPool(), mVulkanInstance.getLogicalDevice());
        createSyncObjects(mVulkanInstance.getLogicalDevice());
        createCamera();
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
        VkBuffer cubeVertexBuffers[] = {mCube.getVertexBuffer()};
        VkBuffer floorVertexBuffers[] = {mFloor.getVertexBuffer()};
        VkDeviceSize offsets[] = {0};

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to start recording command buffers");
        }

        VkRenderPassBeginInfo shadowMapRenderPassInfo{};
        shadowMapRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowMapRenderPassInfo.renderPass = mShadowMapRenderPass;
        shadowMapRenderPassInfo.framebuffer = mShadowMapFramebuffers[imageIndex];
        shadowMapRenderPassInfo.renderArea.offset = {0, 0};
        shadowMapRenderPassInfo.renderArea.extent = mSwapChainExtent;
        VkClearValue shadowMapClearValue{};
        shadowMapClearValue.depthStencil = {1.0f, 0};
        shadowMapRenderPassInfo.clearValueCount = 1;
        shadowMapRenderPassInfo.pClearValues = &shadowMapClearValue;
        vkCmdBeginRenderPass(commandBuffer, &shadowMapRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowMapGraphicsPipeline);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, cubeVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mCube.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadowMapPipelineLayout, 0, 1, &mShadowMapDescriptorSets[currentFrameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mCube.getIndices().size()), 1, 0, 0, 0);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, floorVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mFloor.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mFloor.getIndices().size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        VkFormat depthFormat = findDepthFormat(physicalDevice);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = mShadowMapImages[imageIndex];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(depthFormat)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = mRenderPass;
        renderPassInfo.framebuffer = mSwapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = mSwapChainExtent;
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mCube.getGraphicsPipeline());
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, cubeVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mCube.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mCube.getPipelineLayout(), 0, 1, &mCubeDescriptorSets[currentFrameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mCube.getIndices().size()), 1, 0, 0, 0);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mFloor.getGraphicsPipeline());
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, floorVertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mFloor.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mFloor.getPipelineLayout(), 0, 1, &mFloorDescriptorSets[currentFrameIndex], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mFloor.getIndices().size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

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

        for (size_t i = 0; i < mSwapChainFramebuffers.size(); i++)
        {
            vkDestroyFramebuffer(logicalDevice, mSwapChainFramebuffers[i], nullptr);
            vkDestroyFramebuffer(logicalDevice, mShadowMapFramebuffers[i], nullptr);
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
        mCube.clearResource(mVulkanInstance);
        mFloor.clearResource(mVulkanInstance);
        vkDestroyPipeline(logicalDevice, mShadowMapGraphicsPipeline, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroyBuffer(logicalDevice, mUniformsBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mUniformBuffersMemory[i], nullptr);
            vkDestroyBuffer(logicalDevice, mShadowMapUniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, mShadowMapUniformBuffersMemory[i], nullptr);
        }
        vkDestroySampler(logicalDevice, mCubeTextureSampler, nullptr);
        vkDestroyImageView(logicalDevice, mCubeTextureImageView, nullptr);
        vkDestroyImage(logicalDevice, mCubeTextureImage, nullptr);
        vkFreeMemory(logicalDevice, mCubeTextureImageMemory, nullptr);
        vkDestroySampler(logicalDevice, mFloorTextureSampler, nullptr);
        vkDestroyImageView(logicalDevice, mFloorTextureImageView, nullptr);
        vkDestroyImage(logicalDevice, mFloorTextureImage, nullptr);
        vkFreeMemory(logicalDevice, mFloorTextureImageMemory, nullptr);

        vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mCubeDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mFloorDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, mShadowMapDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(logicalDevice, mShadowMapPipelineLayout, nullptr);
        vkDestroyRenderPass(logicalDevice, mShadowMapRenderPass, nullptr);
        vkDestroyRenderPass(logicalDevice, mRenderPass, nullptr);
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
    VkRenderPass mShadowMapRenderPass;
    VkRenderPass mRenderPass;
    VkDescriptorSetLayout mCubeDescriptorSetLayout;
    VkDescriptorSetLayout mFloorDescriptorSetLayout;
    VkDescriptorSetLayout mShadowMapDescriptorSetLayout;
    VkPipelineLayout mShadowMapPipelineLayout;
    VkPipeline mShadowMapGraphicsPipeline;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    std::vector<VkFramebuffer> mShadowMapFramebuffers;
    std::vector<VkCommandBuffer> mCommandBuffers;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishedSemaphores;
    std::vector<VkFence> mInFlightFences;
    uint32_t mCurrentFrame = 0;
    bool framebufferResized = false;
    Cube mCube;
    Floor mFloor;
    uint32_t mMipLevels;
    VkImage mCubeTextureImage;
    VkImageView mCubeTextureImageView;
    VkSampler mCubeTextureSampler;
    VkDeviceMemory mCubeTextureImageMemory;
    VkImage mFloorTextureImage;
    VkImageView mFloorTextureImageView;
    VkSampler mFloorTextureSampler;
    VkDeviceMemory mFloorTextureImageMemory;
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
    std::vector<VkDescriptorSet> mCubeDescriptorSets;
    std::vector<VkDescriptorSet> mFloorDescriptorSets;
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
