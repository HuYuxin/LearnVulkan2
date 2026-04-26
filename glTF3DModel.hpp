#ifndef GLTF3DMODEL_HPP
#define GLTF3DMODEL_HPP
#include "Frustum.hpp"
#include "VulkanInstance.hpp"
#include "utility.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "external/tinygltf/tiny_gltf.h"

class glTF3DModel {
public:
    glTF3DModel(VulkanInstance* vulkanInstance);
    ~glTF3DModel();
    void initialize(const std::string& filePath);
    bool isInitialized() const { return mInitialized; }
    VkVertexInputBindingDescription2EXT getBindingDescription2EXT();
    std::array<VkVertexInputAttributeDescription2EXT, 4> getAttributeDescriptions2EXT();
    void createVertexBuffer();
    void createIndexBuffer();
    void setupDescriptors(const uint8_t framesInFlightCount,
                        const std::vector<VkBuffer>& uniformBuffers,
                        const VkDeviceSize uboSize,
                        const std::vector<VkImageView>& shadowMapImageViews,
                        const VkSampler& shadowMapSampler);
    const VkDescriptorSet* getUBODescriptorSet(const uint8_t framesInFlightIndex) const;
    const VkDescriptorSet* getShadowMapDescriptorSet(const uint8_t framesInFlightIndex) const;
    void updateShadowMapDescriptorSets(
        const uint8_t framesInFlightCount,
        const std::vector<VkImageView>& shadowMapImageViews,
        const VkSampler& shadowMapSampler);
    
    std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts();
    
    void clearResource();

    int getPrimitiveCount() const { return mPrimitiveCount; }
    
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const uint32_t framesInFlightIndex, const Frustum* frustum, bool isShadowMapPass);

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec4 tangent;

        static VkVertexInputBindingDescription2EXT getBindingDescription2EXT() {
            VkVertexInputBindingDescription2EXT bindingDescription2{};
            bindingDescription2.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
            bindingDescription2.binding = 0;
            bindingDescription2.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            bindingDescription2.stride = sizeof(Vertex);
            bindingDescription2.divisor = 1;
            return bindingDescription2;
        }

        static std::array<VkVertexInputAttributeDescription2EXT, 4> getAttributeDescriptions2EXT() {
            std::array<VkVertexInputAttributeDescription2EXT, 4> attributeDescriptions{};
            attributeDescriptions[0].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            attributeDescriptions[0].pNext = nullptr;
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            attributeDescriptions[1].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            attributeDescriptions[1].pNext = nullptr;
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, normal);
            attributeDescriptions[2].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            attributeDescriptions[2].pNext = nullptr;
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, uv);
            attributeDescriptions[3].sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
            attributeDescriptions[3].pNext = nullptr;
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, tangent);
            return attributeDescriptions;
        }
    };

    struct VertexGPUData
    {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
    };

    struct IndexGPUData
    {
        int count;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
    };

    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
    };

    struct Node {
        Node* parent;
        std::vector<Node*> children;
        Mesh mesh;
        glm::mat4 localTransform;
        ~Node() {
            for (Node* child : children) {
                delete child;
            }
        }
    };

    struct Material {
        int32_t baseColorTextureIndex;
        int32_t metallicRoughnessTextureIndex;
        int32_t normalTextureIndex;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    struct TextureGPUData {
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        VkSampler sampler;
        VkDescriptorImageInfo descriptorInfo;

        void createTextureGPUData(VulkanInstance* vulkanInstance, unsigned char* imageData, VkDeviceSize imageSize, uint32_t width, uint32_t height, bool createMipmaps) {
            if (imageData == nullptr) {
                throw std::runtime_error("Can't create texture from null image data!");
            }

            uint32_t mipLevels = createMipmaps ? (static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))))) + 1 : 1;

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            vulkanInstance->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
            void* data;
            vkMapMemory(vulkanInstance->getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, imageData, static_cast<size_t>(imageSize));
            vkUnmapMemory(vulkanInstance->getLogicalDevice(), stagingBufferMemory);
            
            // Create VkImage
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateImage(vulkanInstance->getLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image!");
            }
        
            // Allocate VkImage memory and bind the memory to VkImage
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(vulkanInstance->getLogicalDevice(), image, &memRequirements);
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = vulkanInstance->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(vulkanInstance->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate image memory!");
            }
            vkBindImageMemory(vulkanInstance->getLogicalDevice(), image, imageMemory, 0);
            
            // transition image layout and copy data from staging buffer to VkImage
            transitionImageLayoutOnetimeSubmit(*vulkanInstance, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
            copyBufferToImage(*vulkanInstance, stagingBuffer, image, width, height);
            if (createMipmaps) {
                generateMipmaps(*vulkanInstance, image, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);
            }

            // Clean up staging buffer
            vkDestroyBuffer(vulkanInstance->getLogicalDevice(), stagingBuffer, nullptr);
            vkFreeMemory(vulkanInstance->getLogicalDevice(), stagingBufferMemory, nullptr);

            // Create image view
            imageView = createImageView(vulkanInstance->getLogicalDevice(), image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        
            // Create sampler
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(vulkanInstance->getPhysicalDevice(), &properties);
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0F;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
            if (vkCreateSampler(vulkanInstance->getLogicalDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture sampler!");
            }

            descriptorInfo.sampler = sampler;
            descriptorInfo.imageView = imageView;
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    };

    struct Image {
        TextureGPUData textureData;
    };

    struct Texture {
        int32_t imageIndex;
    };

    std::vector<Image> mImages;
    std::vector<Texture> mTextures;
    std::vector<Material> mMaterials;
    std::vector<Node*> mNodes;
    std::vector<Vertex> mVertexBufferData;
    VertexGPUData mVertexGPUData;
    std::vector<uint32_t> mIndexBufferData;
    IndexGPUData mIndexGPUData;
    VulkanInstance* mVulkanInstance;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mUBODescriptorSetLayout;
    VkDescriptorSetLayout mShadowMapDescriptorSetLayout;
    VkDescriptorSetLayout mMaterialDescriptorSetLayout;
    std::vector<VkDescriptorSet> mUBODescriptorSet;
    std::vector<VkDescriptorSet> mShadowMapDescriptorSet;
    bool mInitialized;
    int mPrimitiveCount;

    void loadglTFFile(std::string filename);
    void loadImages(tinygltf::Model* glTFInput);
    void loadTextures(tinygltf::Model* glTFInput);
    void loadMaterials(tinygltf::Model* glTFInput);
    void loadScenes(tinygltf::Model* glTFInput);
    void loadNode(const tinygltf::Node* node, const tinygltf::Model* glTFInput, Node* parentNode, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glTF3DModel::Node* node, const uint32_t framesInFlightIndex, const Frustum* frustum, bool isShadowMapPass);
};
#endif
