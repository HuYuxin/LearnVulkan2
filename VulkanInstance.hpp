#ifndef VULKANINSTANCE_HPP
#define VULKANINSTANCE_HPP
#include <GLFW/glfw3.h>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <fstream>


static std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanInstance {
public:
    VulkanInstance();
    void initialize(const bool enableValidationLayer, GLFWwindow* window);
    void destroy();
    VkInstance getInstance() const;
    VkPhysicalDevice getPhysicalDevice() const;
    VkDevice getLogicalDevice() const;
    VkSurfaceKHR getSurface() const;
    VkCommandPool getCommandPool() const;
    VkQueue getGraphicsQueue() const;
    VkQueue getPresentationQueue() const;
    const VkSampleCountFlagBits getMsaaSamples() const;
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(const VkCommandBuffer& commandBuffer);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::vector<char> &code);



private:
    void createVulkanInstance();
    std::vector<const char *> getRequiredExtensions();
    void setupDebugMessenger();
    bool isDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice();
    void createSurface();
    void createLogicalDevice();
    void createCommandPool();
    VkSampleCountFlagBits getMaxUsableSampleCount();

    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    GLFWwindow* mWindow;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice;
    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;
    VkSurfaceKHR mSurface;
    VkCommandPool mCommandPool;
    VkSampleCountFlagBits mMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    bool mEnableValidationLayer;
};

#endif