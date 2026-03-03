#ifndef OBJECT_HPP
#define OBJECT_HPP
#include "Vertex.hpp"
#include "VulkanInstance.hpp"

class Object {
public:
    Object();
    virtual void initializeObject() = 0;
    virtual void createTextures(VulkanInstance& vulkanInstance) = 0;
    virtual void createVertexBuffer(VulkanInstance& vulkanInstance) = 0;
    virtual void createIndexBuffer(VulkanInstance& vulkanInstance) = 0;
    virtual void updateShadowMapDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) = 0;
    virtual void createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const VkDescriptorSetLayout descriptorSetLayout,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) = 0;
    virtual const VkBuffer& getVertexBuffer() const;
    virtual const VkBuffer& getIndexBuffer() const;
    virtual std::vector<uint32_t> getIndices() const;
    virtual const std::vector<VkDescriptorSet>& getDescriptorSets() const;
    virtual void clearResource(VulkanInstance& vulkanInstance);
    
protected:
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkBuffer mIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory mIndexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> mDescriptorSets;
};
#endif