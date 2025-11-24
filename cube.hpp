#ifndef CUBE_HPP
#define CUBE_HPP
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Vertex.hpp"
#include "vulkanInstance.hpp"

class Cube {
public:
    Cube();
    void createCubeVertexBuffer(VulkanInstance& vulkanInstance);
    void createCubeIndexBuffer(VulkanInstance& vulkanInstance);
    void createCubeTextureImage(VulkanInstance& vulkanInstance);
    void createCubeTextureImageView(VulkanInstance& vulkanInstance);
    void createCubeTextureSampler(VulkanInstance& vulkanInstance);
    void createDescriptorSetLayout(VulkanInstance& vulkanInstance);
    void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler);
    void createGraphicsPipeline(VulkanInstance& vulkanInstance, const VkExtent2D swapChainExtent, const VkRenderPass renderPass);
    const VkBuffer& getVertexBuffer() const;
    const VkBuffer& getIndexBuffer() const;
    const VkImage& getCubeTextureImage() const;
    const VkImageView& getCubeTextureImageView() const;
    const VkSampler& getCubeTextureSampler() const;
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
    VkDeviceMemory mCubeTextureImageMemory;
    VkImage mCubeTextureImage;
    VkImageView mCubeTextureImageView;
    VkSampler mCubeTextureSampler;
    uint32_t mMipLevels;
    VkDescriptorSetLayout mCubeDescriptorSetLayout;
    std::vector<VkDescriptorSet> mCubeDescriptorSets;
};
#endif