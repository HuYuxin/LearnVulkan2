#include "Object.hpp"

Object::Object() {
    mVertexBuffer = VK_NULL_HANDLE;
    mIndexBuffer = VK_NULL_HANDLE;
    mVertexBufferMemory = VK_NULL_HANDLE;
    mIndexBufferMemory = VK_NULL_HANDLE;
}

const VkBuffer& Object::getVertexBuffer() const {
    return mVertexBuffer;
}

const VkBuffer& Object::getIndexBuffer() const {
    return mIndexBuffer;
}

std::vector<uint32_t> Object::getIndices() const {
    return mIndices;
}

const std::vector<VkDescriptorSet>& Object::getDescriptorSets() const {
    return mDescriptorSets;
}

void Object::clearResource(VulkanInstance& vulkanInstance) {
    VkDevice device = vulkanInstance.getLogicalDevice();
    if (mVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mVertexBuffer, nullptr);
        mVertexBuffer = VK_NULL_HANDLE;
    }
    if (mVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mVertexBufferMemory, nullptr);
        mVertexBufferMemory = VK_NULL_HANDLE;
    }
    if (mIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, mIndexBuffer, nullptr);
        mIndexBuffer = VK_NULL_HANDLE;
    }
    if (mIndexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, mIndexBufferMemory, nullptr);
        mIndexBufferMemory = VK_NULL_HANDLE;
    }
}
