#ifndef FLOOR_HPP
#define FLOOR_HPP
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Object.hpp"
#include "Vertex.hpp"
#include "VulkanInstance.hpp"

namespace {
    const std::string FLOOR_TEXTURE_PATH = "textures/floor.jpg";
}

class Floor : public Object{
public:
    Floor() : Object() {};
    virtual void initializeObject() override;
    virtual void createVertexBuffer(VulkanInstance& vulkanInstance) override;
    virtual void createIndexBuffer(VulkanInstance& vulkanInstance) override;
    virtual void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const VkDescriptorSetLayout descriptorSetLayout,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) override;
    virtual void clearResource(VulkanInstance& vulkanInstance) override;
    virtual void createTextures(VulkanInstance& vulkanInstance) override;

private:
    VkDeviceMemory mFloorTextureImageMemory;
    VkImage mFloorTextureImage;
    VkImageView mFloorTextureImageView;
    VkSampler mFloorTextureSampler;
    uint32_t mMipLevels;

    void createFloorTextureImage(VulkanInstance& vulkanInstance);
    void createFloorTextureImageView(VulkanInstance& vulkanInstance);
    void createFloorTextureSampler(VulkanInstance& vulkanInstance);
};
#endif
