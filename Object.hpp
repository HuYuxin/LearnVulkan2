#ifndef OBJECT_HPP
#define OBJECT_HPP
#include <vulkan/vulkan_core.h>
#include "Vertex.hpp"
#include "vulkanInstance.hpp"

class Object {
public:
    Object();
    virtual void initializeObject() = 0;
    virtual void createTextures(VulkanInstance& vulkanInstance) = 0;
    virtual void createVertexBuffer(VulkanInstance& vulkanInstance) = 0;
    virtual void createIndexBuffer(VulkanInstance& vulkanInstance) = 0;
    virtual void createDescriptorSetLayout(VulkanInstance& vulkanInstance) = 0;
    virtual void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) = 0;
    virtual void createGraphicsPipeline(VulkanInstance& vulkanInstance, const VkExtent2D swapChainExtent, const VkRenderPass renderPass) = 0;
    virtual const VkBuffer& getVertexBuffer() const;
    virtual const VkBuffer& getIndexBuffer() const;
    virtual std::vector<uint32_t> getIndices() const;
    virtual const VkPipeline getGraphicsPipeline() const;
    virtual const VkPipelineLayout getPipelineLayout() const;
    virtual const std::vector<VkDescriptorSet>& getDescriptorSets() const;
    virtual void clearResource(VulkanInstance& vulkanInstance);
    
protected:
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    VkBuffer mVertexBuffer;
    VkBuffer mIndexBuffer;
    VkDeviceMemory mVertexBufferMemory;
    VkDeviceMemory mIndexBufferMemory;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;
};
#endif