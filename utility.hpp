#ifndef UTILITY_HPP
#define UTILITY_HPP
#include "vulkanInstance.hpp"
#include <vulkan/vulkan_core.h>


void createImage(VulkanInstance& vulkanInstance, 
                const VkDevice& logicalDevice,
                uint32_t width,
                uint32_t height,
                uint32_t mipLevels,
                VkSampleCountFlagBits numSamples,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkImage& image,
                VkDeviceMemory& imageMemory);

void transitionImageLayout(VulkanInstance& vulkanInstance, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

bool hasStencilComponent(VkFormat format);

void copyBufferToImage(VulkanInstance& instance, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void generateMipmaps(VulkanInstance& instance, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

VkImageView createImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

#endif