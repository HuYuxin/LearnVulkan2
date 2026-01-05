#ifndef CUBE_HPP
#define CUBE_HPP
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Object.hpp"
#include "Vertex.hpp"
#include "VulkanInstance.hpp"

class Cube : public Object {
public:
    Cube() : Object() {};
    virtual void initializeObject() override;
    virtual void createVertexBuffer(VulkanInstance& vulkanInstance) override;
    virtual void createIndexBuffer(VulkanInstance& vulkanInstance);
    virtual void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const VkDescriptorSetLayout descriptorSetLayout,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) override;
    virtual void clearResource(VulkanInstance& vulkanInstance) override;
    virtual void createTextures(VulkanInstance& vulkanInstance) override;
    
private:
    VkDeviceMemory mCubeTextureImageMemory;
    VkImage mCubeTextureImage;
    VkImageView mCubeTextureImageView;
    VkSampler mCubeTextureSampler;
    uint32_t mMipLevels;

    void createCubeTextureImage(VulkanInstance& vulkanInstance);
    void createCubeTextureImageView(VulkanInstance& vulkanInstance);
    void createCubeTextureSampler(VulkanInstance& vulkanInstance);
};
#endif
