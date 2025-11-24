#ifndef FLOOR_HPP
#define FLOOR_HPP
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Vertex.hpp"
#include "vulkanInstance.hpp"

namespace {
    const std::string FLOOR_TEXTURE_PATH = "textures/floor.jpg";
}

class Floor {
public:
    Floor();
    void createFloorVertexBuffer(VulkanInstance& vulkanInstance);
    void createFloorIndexBuffer(VulkanInstance& vulkanInstance);
    void createGraphicsPipeline(VulkanInstance& vulkanInstance, const VkExtent2D swapChainExtent, const VkRenderPass renderPass);
    void createFloorTextureImage(VulkanInstance& vulkanInstance);
    void createFloorTextureImageView(VulkanInstance& vulkanInstance);
    void createFloorTextureSampler(VulkanInstance& vulkanInstance);
    void createDescriptorSetLayout(VulkanInstance& vulkanInstance);
    void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler);
    const VkBuffer& getVertexBuffer() const;
    const VkBuffer& getIndexBuffer() const;
    const VkImage& getFloorTextureImage() const;
    const VkImageView& getFloorTextureImageView() const;
    const VkSampler& getFloorTextureSampler() const;
    void clearResource(VulkanInstance& vulkanInstance);
    std::vector<uint32_t> getIndices() const;
    const VkPipeline getGraphicsPipeline() const;
    const VkPipelineLayout getPipelineLayout() const;
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;

private:
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    VkBuffer mVertexBuffer;
    VkBuffer mIndexBuffer;
    VkDeviceMemory mVertexBufferMemory;
    VkDeviceMemory mIndexBufferMemory;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    VkDeviceMemory mFloorTextureImageMemory;
    VkImage mFloorTextureImage;
    VkImageView mFloorTextureImageView;
    VkSampler mFloorTextureSampler;
    uint32_t mMipLevels;
    VkDescriptorSetLayout mFloorDescriptorSetLayout;
    std::vector<VkDescriptorSet> mFloorDescriptorSets;
};
#endif