#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "glTF3DModel.hpp"
#include <iostream>

glTF3DModel::glTF3DModel(VulkanInstance* vulkanInstance) : mVulkanInstance(vulkanInstance), mInitialized(false) {
    mVertexGPUData.vertexBuffer = VK_NULL_HANDLE;
    mVertexGPUData.vertexBufferMemory = VK_NULL_HANDLE;
    mIndexGPUData.indexBuffer = VK_NULL_HANDLE;
    mIndexGPUData.indexBufferMemory = VK_NULL_HANDLE;
    mDescriptorPool = VK_NULL_HANDLE;
    mUBODescriptorSetLayout = VK_NULL_HANDLE;
    mShadowMapDescriptorSetLayout = VK_NULL_HANDLE;
    mTextureDescriptorSetLayout = VK_NULL_HANDLE;

}

void glTF3DModel::clearResource() {
    vkDestroyBuffer(mVulkanInstance->getLogicalDevice(), mVertexGPUData.vertexBuffer, nullptr);
    vkFreeMemory(mVulkanInstance->getLogicalDevice(), mVertexGPUData.vertexBufferMemory, nullptr);
    vkDestroyBuffer(mVulkanInstance->getLogicalDevice(), mIndexGPUData.indexBuffer, nullptr);
    vkFreeMemory(mVulkanInstance->getLogicalDevice(), mIndexGPUData.indexBufferMemory, nullptr);
    for (Image& image : mImages) {
        vkDestroySampler(mVulkanInstance->getLogicalDevice(), image.textureData.sampler, nullptr);
        vkDestroyImageView(mVulkanInstance->getLogicalDevice(), image.textureData.imageView, nullptr);
        vkDestroyImage(mVulkanInstance->getLogicalDevice(), image.textureData.image, nullptr);
        vkFreeMemory(mVulkanInstance->getLogicalDevice(), image.textureData.imageMemory, nullptr);
    }
    vkDestroyDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), mUBODescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), mShadowMapDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), mTextureDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(mVulkanInstance->getLogicalDevice(), mDescriptorPool, nullptr);

    mVertexBufferData.clear();
    mIndexBufferData.clear();
    mImages.clear();
    mTextures.clear();
    mMaterials.clear();
    for (Node* node: mNodes) {
        delete node;
    }
    mNodes.clear();
    mUBODescriptorSet.clear();
    mShadowMapDescriptorSet.clear();
    mInitialized = false;
}

glTF3DModel::~glTF3DModel() {
    for (Node* node: mNodes) {
        delete node;
    }
}

void glTF3DModel::initialize(const std::string& filePath) {
    loadglTFFile(filePath);
    mInitialized = true;
}

void glTF3DModel::loadglTFFile(std::string filename)
{
	tinygltf::Model glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

    bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

    if (fileLoaded) {
        loadImages(&glTFInput);
        loadMaterials(&glTFInput);
        loadTextures(&glTFInput);
        loadScenes(&glTFInput);
    } else {
        throw std::runtime_error("Failed to load glTF file: " + error);
    }
}

void glTF3DModel::loadImages(tinygltf::Model* glTFInput) {
    mImages.resize(glTFInput->images.size());
    for (size_t i = 0; i <glTFInput->images.size(); ++i) {
        const tinygltf::Image& glTFImage = glTFInput->images[i];
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;
        if (glTFImage.component == 3) {
            bufferSize = glTFImage.width * glTFImage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            const unsigned char* rgb = &glTFImage.image[0];

            for (size_t j = 0; j < glTFImage.width * glTFImage.height; ++j) {
                memcpy (rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        } else {
            buffer = const_cast<unsigned char*>(&glTFImage.image[0]);
            bufferSize = glTFImage.image.size();
        }

        mImages[i].textureData.createTextureGPUData(mVulkanInstance, buffer, bufferSize, glTFImage.width, glTFImage.height, true);
        if (deleteBuffer) {
            delete[] buffer;
        }
    }
}

void glTF3DModel::loadTextures(tinygltf::Model* glTFInput) {
    mTextures.resize(glTFInput->textures.size());
    for (size_t i = 0; i < glTFInput->textures.size(); ++i) {
        const tinygltf::Texture& glTFTexture = glTFInput->textures[i];
        Texture texture = {};
        texture.imageIndex = glTFTexture.source;
        mTextures[i] = texture;
    }
}

void glTF3DModel::loadMaterials(tinygltf::Model* glTFInput) {
    mMaterials.resize(glTFInput->materials.size());
    for (size_t i = 0; i < glTFInput->materials.size(); ++i) {
        const tinygltf::Material& glTFMaterial = glTFInput->materials[i];
        Material material = {};
        if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
            material.baseColorFactor = glm::make_vec4(glTFMaterial.values.at("baseColorFactor").ColorFactor().data());
        } else {
            material.baseColorFactor = glm::vec4(1.0f);
        }
        if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
            material.baseColorTextureIndex = glTFMaterial.values.at("baseColorTexture").TextureIndex();
        } else {
            material.baseColorTextureIndex = -1;
        }
        mMaterials[i] = material;
    }
}

void glTF3DModel::loadScenes(tinygltf::Model* glTFInput) {
    const tinygltf::Scene& scene = glTFInput->scenes[0];
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        const tinygltf::Node node = glTFInput->nodes[scene.nodes[i]];
        loadNode(&node, glTFInput, nullptr, mIndexBufferData, mVertexBufferData);
    }
}

void glTF3DModel::loadNode(const tinygltf::Node* gltfNode, const tinygltf::Model* glTFInput, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    Node* newNode = new Node();
    newNode->parent = parent;
    glm::mat4 translationMat(1.0f), rotationMat(1.0f), scaleMat(1.0f);

    if (gltfNode->translation.size() == 3) {
        translationMat = glm::translate(glm::mat4(1.0f), glm::vec3(gltfNode->translation[0], gltfNode->translation[1], gltfNode->translation[2]));
    }

    if (gltfNode->scale.size() == 3) {
        scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(gltfNode->scale[0], gltfNode->scale[1], gltfNode->scale[2]));
    }

    if (gltfNode->rotation.size() == 4) {
        // glTF stores quaternion as [x, y, z, w]; glm::quat constructor takes (w, x, y, z)
        glm::quat rotation = glm::quat(gltfNode->rotation[3], gltfNode->rotation[0], gltfNode->rotation[1], gltfNode->rotation[2]);
        rotationMat = glm::mat4(rotation);
    }

    // glTF TRS order: T * R * S
    newNode->localTransform = translationMat * rotationMat * scaleMat;

    if (gltfNode->matrix.size() == 16) {
        newNode->localTransform = glm::make_mat4(gltfNode->matrix.data());
    }

    if (gltfNode->children.size() > 0) {
        for (size_t i = 0; i < gltfNode->children.size(); ++i) {
            loadNode(&glTFInput->nodes[gltfNode->children[i]], glTFInput, newNode, indexBuffer, vertexBuffer);
        }
    }

    if (gltfNode->mesh >= 0) {
        const tinygltf::Mesh mesh = glTFInput->meshes[gltfNode->mesh];
        for (size_t i = 0; i < mesh.primitives.size(); ++i) {
            const tinygltf::Primitive& primitive = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;

            // Load Vertex
            const float* positionBuffer = nullptr;
            const float* normalBuffer = nullptr;
            const float* texcoordBuffer = nullptr;

            size_t vertexCount = 0;

            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                const tinygltf::Accessor& positionAccessor = glTFInput->accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& positionBufferView = glTFInput->bufferViews[positionAccessor.bufferView];
                const tinygltf::Buffer& positionBufferData = glTFInput->buffers[positionBufferView.buffer];
                    
                positionBuffer = reinterpret_cast<const float*>(positionBufferData.data.data() + positionAccessor.byteOffset + positionBufferView.byteOffset);
                vertexCount = positionAccessor.count;
            }

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const tinygltf::Accessor& normalAccessor = glTFInput->accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& normalBufferView = glTFInput->bufferViews[normalAccessor.bufferView];
                const tinygltf::Buffer& normalBufferData = glTFInput->buffers[normalBufferView.buffer];
                normalBuffer = reinterpret_cast<const float*>(normalBufferData.data.data() + normalAccessor.byteOffset + normalBufferView.byteOffset);
            }

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const tinygltf::Accessor& texcoordAccessor = glTFInput->accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& texcoordBufferView = glTFInput->bufferViews[texcoordAccessor.bufferView];
                const tinygltf::Buffer& texcoordBufferData = glTFInput->buffers[texcoordBufferView.buffer];
                texcoordBuffer = reinterpret_cast<const float*>(texcoordBufferData.data.data() + texcoordAccessor.byteOffset + texcoordBufferView.byteOffset);
            }

            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                Vertex vertex = {};
                vertex.pos = glm::vec3(positionBuffer[vertexIndex * 3], positionBuffer[vertexIndex * 3 + 1], positionBuffer[vertexIndex * 3 + 2]);
                if (normalBuffer != nullptr) {
                    vertex.normal = glm::vec3(normalBuffer[vertexIndex * 3], normalBuffer[vertexIndex * 3 + 1], normalBuffer[vertexIndex * 3 + 2]);
                }
                if (texcoordBuffer != nullptr) {
                    vertex.uv = glm::vec2(texcoordBuffer[vertexIndex * 2], texcoordBuffer[vertexIndex * 2 + 1]);
                }
                vertexBuffer.push_back(vertex);
            }

            // Load index
            const tinygltf::Accessor& indexAccessor = glTFInput->accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = glTFInput->bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBufferData = glTFInput->buffers[indexBufferView.buffer];
            indexCount = static_cast<uint32_t>(indexAccessor.count);

            switch (indexAccessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(&indexBufferData.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
					for (size_t index = 0; index < indexCount; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(&indexBufferData.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
					for (size_t index = 0; index < indexCount; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(&indexBufferData.data[indexAccessor.byteOffset + indexBufferView.byteOffset]);
					for (size_t index = 0; index < indexCount; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default: {
                    std::runtime_error("Unsupported index component type!");
					return;
                }
			}

            Primitive newPrimitive = {};
            newPrimitive.firstIndex = firstIndex;
            newPrimitive.indexCount = indexCount;
            newPrimitive.materialIndex = primitive.material;
            newNode->mesh.primitives.push_back(newPrimitive);
        }
    }

    if (parent == nullptr) {
        mNodes.push_back(newNode);
    } else {
        parent->children.push_back(newNode);
    }
}

VkVertexInputBindingDescription2EXT glTF3DModel::getBindingDescription2EXT() {
    return Vertex::getBindingDescription2EXT();
}

std::array<VkVertexInputAttributeDescription2EXT, 3> glTF3DModel::getAttributeDescriptions2EXT() {
    return Vertex::getAttributeDescriptions2EXT();
}

void glTF3DModel::createVertexBuffer() {
    VkDeviceSize vertexBufferSize = sizeof(mVertexBufferData[0]) * mVertexBufferData.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    mVulkanInstance->createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, mVertexBufferData.data(), vertexBufferSize);
    vkUnmapMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory);

    mVulkanInstance->createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexGPUData.vertexBuffer, mVertexGPUData.vertexBufferMemory);
    
    mVulkanInstance->copyBuffer(stagingBuffer, mVertexGPUData.vertexBuffer, vertexBufferSize);
    vkDestroyBuffer(mVulkanInstance->getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory, nullptr);
}

void glTF3DModel::createIndexBuffer() {
    VkDeviceSize indexBufferSize = sizeof(mIndexBufferData[0]) * mIndexBufferData.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    mVulkanInstance->createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, mIndexBufferData.data(), indexBufferSize);
    vkUnmapMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory);

    mVulkanInstance->createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexGPUData.indexBuffer, mIndexGPUData.indexBufferMemory);
    
    mVulkanInstance->copyBuffer(stagingBuffer, mIndexGPUData.indexBuffer, indexBufferSize);
    vkDestroyBuffer(mVulkanInstance->getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(mVulkanInstance->getLogicalDevice(), stagingBufferMemory, nullptr);
}

void glTF3DModel::setupDescriptors(const uint8_t framesInFlightCount,
                        const std::vector<VkBuffer>& uniformBuffers,
                        const VkDeviceSize uboSize,
                        const std::vector<VkImageView>& shadowMapImageViews,
                        const VkSampler& shadowMapSampler) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    // 1 descriptor set for ubo, per frame in flight
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = framesInFlightCount;
    // 1 descriptor set for shadow map and 1 descriptor set for each texture, per frame in flight
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(mImages.size() + 1) * framesInFlightCount;
    const uint32_t maxDescriptorSetCount = (2 + static_cast<uint32_t>(mImages.size())) * framesInFlightCount; 

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxDescriptorSetCount;
    if(vkCreateDescriptorPool(mVulkanInstance->getLogicalDevice(), &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetLayoutBinding uboDescriptorSetLayoutBinding{};
    uboDescriptorSetLayoutBinding.binding = 0;
    uboDescriptorSetLayoutBinding.descriptorCount = 1;
    uboDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboDescriptorSetLayoutBinding;
    if (vkCreateDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), &uboLayoutInfo, nullptr, &mUBODescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    std::vector<VkDescriptorSetLayout> uboDescriptorSetLayouts(framesInFlightCount, mUBODescriptorSetLayout);
    VkDescriptorSetAllocateInfo uboAllocInfo {};
    uboAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    uboAllocInfo.descriptorPool = mDescriptorPool;
    uboAllocInfo.descriptorSetCount = framesInFlightCount;
    uboAllocInfo.pSetLayouts = uboDescriptorSetLayouts.data();
    mUBODescriptorSet.resize(framesInFlightCount);
    if(vkAllocateDescriptorSets(mVulkanInstance->getLogicalDevice(), &uboAllocInfo, mUBODescriptorSet.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    for (size_t i = 0; i < framesInFlightCount; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = uboSize;

        VkWriteDescriptorSet uboDescriptorWrite{};
        uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboDescriptorWrite.dstSet = mUBODescriptorSet[i];
        uboDescriptorWrite.dstBinding = 0;
        uboDescriptorWrite.dstArrayElement = 0;
        uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescriptorWrite.descriptorCount = 1;
        uboDescriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(mVulkanInstance->getLogicalDevice(), 1, &uboDescriptorWrite, 0, nullptr);
    }

    VkDescriptorSetLayoutBinding shadowMapDescriptorSetLayoutBinding{};
    shadowMapDescriptorSetLayoutBinding.binding = 0;
    shadowMapDescriptorSetLayoutBinding.descriptorCount = 1;
    shadowMapDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowMapDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo shadowMapLayoutInfo{};
    shadowMapLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    shadowMapLayoutInfo.bindingCount = 1;
    shadowMapLayoutInfo.pBindings = &shadowMapDescriptorSetLayoutBinding;
    if (vkCreateDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), &shadowMapLayoutInfo, nullptr, &mShadowMapDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
	
    std::vector<VkDescriptorSetLayout> shadowMapDescriptorSetLayouts(framesInFlightCount, mShadowMapDescriptorSetLayout);
    VkDescriptorSetAllocateInfo shadowMapAllocInfo {};
    shadowMapAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    shadowMapAllocInfo.descriptorPool = mDescriptorPool;
    shadowMapAllocInfo.descriptorSetCount = framesInFlightCount;
    shadowMapAllocInfo.pSetLayouts = shadowMapDescriptorSetLayouts.data();
    mShadowMapDescriptorSet.resize(framesInFlightCount);
    if(vkAllocateDescriptorSets(mVulkanInstance->getLogicalDevice(), &shadowMapAllocInfo, mShadowMapDescriptorSet.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    for (size_t i = 0; i < framesInFlightCount; ++i) {
        VkDescriptorImageInfo shadowMapInfo{};
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = shadowMapImageViews[i];
        shadowMapInfo.sampler = shadowMapSampler;

        VkWriteDescriptorSet shadowMapDescriptorWrite{};
        shadowMapDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowMapDescriptorWrite.dstSet = mShadowMapDescriptorSet[i];
        shadowMapDescriptorWrite.dstBinding = 0;
        shadowMapDescriptorWrite.dstArrayElement = 0;
        shadowMapDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapDescriptorWrite.descriptorCount = 1;
        shadowMapDescriptorWrite.pImageInfo = &shadowMapInfo;

        vkUpdateDescriptorSets(mVulkanInstance->getLogicalDevice(), 1, &shadowMapDescriptorWrite, 0, nullptr);
    }

    VkDescriptorSetLayoutBinding textureDescriptorSetLayoutBinding{};
    textureDescriptorSetLayoutBinding.binding = 0;
    textureDescriptorSetLayoutBinding.descriptorCount = 1;
    textureDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureDescriptorSetLayoutBinding;
    if (vkCreateDescriptorSetLayout(mVulkanInstance->getLogicalDevice(), &textureLayoutInfo, nullptr, &mTextureDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo textureAllocInfo {};
    textureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    textureAllocInfo.descriptorPool = mDescriptorPool;
    textureAllocInfo.descriptorSetCount = framesInFlightCount;
    std::vector<VkDescriptorSetLayout> textureDescriptorSetLayouts(framesInFlightCount, mTextureDescriptorSetLayout);
    textureAllocInfo.pSetLayouts = textureDescriptorSetLayouts.data();
    for (Image& image : mImages) {
        image.descriptorSet.resize(framesInFlightCount);
        if(vkAllocateDescriptorSets(mVulkanInstance->getLogicalDevice(), &textureAllocInfo, image.descriptorSet.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            VkWriteDescriptorSet textureDescriptorWrite{};
            textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureDescriptorWrite.dstSet = image.descriptorSet[i];
            textureDescriptorWrite.dstBinding = 0;
            textureDescriptorWrite.dstArrayElement = 0;
            textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureDescriptorWrite.descriptorCount = 1;
            textureDescriptorWrite.pImageInfo = &image.textureData.descriptorInfo;
            vkUpdateDescriptorSets(mVulkanInstance->getLogicalDevice(), 1, &textureDescriptorWrite, 0, nullptr);
        }
    }
}

void glTF3DModel:: updateShadowMapDescriptorSets(
    const uint8_t framesInFlightCount,
    const std::vector<VkImageView>& shadowMapImageViews,
    const VkSampler& shadowMapSampler) {
    for (size_t i = 0; i < framesInFlightCount; i++) {
        VkDescriptorImageInfo shadowMapInfo{};
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = shadowMapImageViews[i];
        shadowMapInfo.sampler = shadowMapSampler;

        VkWriteDescriptorSet shadowMapWrite{};
        shadowMapWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        shadowMapWrite.dstSet = mShadowMapDescriptorSet[i];
        shadowMapWrite.dstBinding = 0;
        shadowMapWrite.dstArrayElement = 0;
        shadowMapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapWrite.descriptorCount = 1;
        shadowMapWrite.pImageInfo = &shadowMapInfo;

        vkUpdateDescriptorSets(mVulkanInstance->getLogicalDevice(), 1, &shadowMapWrite, 0, nullptr);
    }
}

const VkDescriptorSet* glTF3DModel::getUBODescriptorSet(const uint8_t framesInFlightIndex) const {
    return &mUBODescriptorSet[framesInFlightIndex];
}

const VkDescriptorSet* glTF3DModel::getShadowMapDescriptorSet(const uint8_t framesInFlightIndex) const {
    return &mShadowMapDescriptorSet[framesInFlightIndex];
}

std::array<VkDescriptorSetLayout, 3> glTF3DModel::getDescriptorSetLayouts()
{
    return {mUBODescriptorSetLayout, mShadowMapDescriptorSetLayout, mTextureDescriptorSetLayout};
}

// Draw a single node including child nodes (if present)
void glTF3DModel::drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Node* node, const uint32_t framesInFlightIndex, bool isShadowMapPass)
{
	if (node->mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node->localTransform;
		Node* currentParent = node->parent;
		while (currentParent) {
			nodeMatrix = currentParent->localTransform * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
		for (Primitive& primitive : node->mesh.primitives) {
			if (primitive.indexCount > 0) {
				// Get the texture index for this primitive
                if (mMaterials[primitive.materialIndex].baseColorTextureIndex >= 0 && !isShadowMapPass) {
                    Texture texture = mTextures[mMaterials[primitive.materialIndex].baseColorTextureIndex];
				    // Bind the descriptor for the current primitive's texture
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &mImages[texture.imageIndex].descriptorSet[framesInFlightIndex], 0, nullptr);
                }
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : node->children) {
		drawNode(commandBuffer, pipelineLayout, child, framesInFlightIndex, isShadowMapPass);
	}
}

// Draw the glTF scene starting at the top-level-nodes
void glTF3DModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, const uint32_t framesInFlightIndex, bool isShadowMapPass)
{
	// All vertices and indices are stored in single buffers, so we only need to bind once
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mVertexGPUData.vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, mIndexGPUData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	// Render all nodes at top-level
	for (auto& node : mNodes) {
		drawNode(commandBuffer, pipelineLayout, node, framesInFlightIndex, isShadowMapPass);
	}
}
