#ifndef FLOOR_HPP
#define FLOOR_HPP
#include <vulkan/vulkan_core.h>
#include <vector>
#include "Vertex.hpp"
#include "vulkanInstance.hpp"

class Floor {
public:
    Floor();
    void createFloorVertexBuffer(VulkanInstance& vulkanInstance);
    void createFloorIndexBuffer(VulkanInstance& vulkanInstance);
    const VkBuffer& getVertexBuffer() const;
    const VkBuffer& getIndexBuffer() const;
    void clearResource(VulkanInstance& vulkanInstance);
    std::vector<uint32_t> getIndices() const;

private:
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    VkBuffer mVertexBuffer;
    VkBuffer mIndexBuffer;
    VkDeviceMemory mVertexBufferMemory;
    VkDeviceMemory mIndexBufferMemory;
};
#endif