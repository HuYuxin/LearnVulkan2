#include "floor.hpp"
#include <string.h>

Floor::Floor(){
        // Floor
        Vertex FloorVertex1{};
        FloorVertex1.pos = glm::vec3(-10.0, -2.0, 10.0);
        FloorVertex1.normal = glm::vec3(0.0, 1.0, 0.0);
        FloorVertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(FloorVertex1);
        Vertex FloorVertex2{};
        FloorVertex2.pos = glm::vec3(10.0, -2.0, 10.0);
        FloorVertex2.normal = glm::vec3(0.0, 1.0, 0.0);
        FloorVertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(FloorVertex2);
        Vertex FloorVertex3{};
        FloorVertex3.pos = glm::vec3(10.0, -2.0, -10.0);
        FloorVertex3.normal = glm::vec3(0.0, 1.0, 0.0);
        FloorVertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(FloorVertex3);
        Vertex FloorVertex4{};
        FloorVertex4.pos = glm::vec3(-10.0, -2.0, -10.0);
        FloorVertex4.normal = glm::vec3(0.0, 1.0, 0.0);
        FloorVertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(FloorVertex4);
        mIndices.push_back(0);
        mIndices.push_back(1);
        mIndices.push_back(2);
        mIndices.push_back(0);
        mIndices.push_back(2);
        mIndices.push_back(3);
}

void Floor::createFloorVertexBuffer(VulkanInstance& vulkanInstance) {
    VkDeviceSize vertexBufferSize = sizeof(mVertices[0]) * mVertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vulkanInstance.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, mVertices.data(), vertexBufferSize);
    vkUnmapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory);

    vulkanInstance.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer, mVertexBufferMemory);
    vulkanInstance.copyBuffer(stagingBuffer, mVertexBuffer, vertexBufferSize);
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, nullptr);    
}

void Floor::createFloorIndexBuffer(VulkanInstance& vulkanInstance) {
    VkDeviceSize indexBufferSize = sizeof(mIndices[0]) * mIndices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vulkanInstance.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingBufferMemory);
        
    void* data;
    vkMapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, mIndices.data(), (size_t) indexBufferSize);
    vkUnmapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory);

    vulkanInstance.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer, mIndexBufferMemory);

    vulkanInstance.copyBuffer(stagingBuffer, mIndexBuffer, indexBufferSize);
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, nullptr);    
}

void Floor::clearResource(VulkanInstance& vulkanInstance) {
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mVertexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mIndexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mIndexBufferMemory, nullptr);
}

const VkBuffer& Floor::getVertexBuffer() const {
    return mVertexBuffer;
}

const VkBuffer& Floor::getIndexBuffer() const {
    return mIndexBuffer;
}

std::vector<uint32_t> Floor::getIndices() const {
    return mIndices;
}
