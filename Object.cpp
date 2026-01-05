#include "Object.hpp"

Object::Object() {

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
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mVertexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mIndexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mIndexBufferMemory, nullptr);
}
