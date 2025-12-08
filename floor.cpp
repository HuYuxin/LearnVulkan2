#include "floor.hpp"
#include "utility.hpp"
#include <stb_image.h>
#include <string.h>

void Floor::initializeObject(){
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

void Floor::createVertexBuffer(VulkanInstance& vulkanInstance) {
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

void Floor::createIndexBuffer(VulkanInstance& vulkanInstance) {
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

void Floor::createGraphicsPipeline(VulkanInstance& vulkanInstance,
                                    const VkExtent2D swapChainExtent,
                                    const VkRenderPass renderPass) {
    auto floorVertShaderCode = readFile("shaders/lambertVert.spv");
    auto floorFragShaderCode = readFile("shaders/lambertFrag.spv");

    VkShaderModule floorVertShaderModule = vulkanInstance.createShaderModule(floorVertShaderCode);
    VkShaderModule floorFragShaderModule = vulkanInstance.createShaderModule(floorFragShaderCode);
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = floorVertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = floorFragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo floorShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::getBindingDescription();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    auto vertexAttributeDescription = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vulkanInstance.getMsaaSamples();
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(vulkanInstance.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = floorShaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(vulkanInstance.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(vulkanInstance.getLogicalDevice(), floorFragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanInstance.getLogicalDevice(), floorVertShaderModule, nullptr);          
}

void Floor::createFloorTextureImage(VulkanInstance& vulkanInstance) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(FLOOR_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    mMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vulkanInstance.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(vulkanInstance, vulkanInstance.getLogicalDevice(), texWidth, texHeight, mMipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mFloorTextureImage, mFloorTextureImageMemory);

    transitionImageLayout(vulkanInstance, mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels);
    copyBufferToImage(vulkanInstance, stagingBuffer, mFloorTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(vulkanInstance, mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mMipLevels);

    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, nullptr);
}

void Floor::createFloorTextureImageView(VulkanInstance& vulkanInstance) {
    mFloorTextureImageView = createImageView(vulkanInstance.getLogicalDevice(), mFloorTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels);
}

void Floor::createFloorTextureSampler(VulkanInstance& VulkanInstance) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(VulkanInstance.getPhysicalDevice(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0F;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    if (vkCreateSampler(VulkanInstance.getLogicalDevice(), &samplerInfo, nullptr, &mFloorTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Floor::createDescriptorSetLayout(VulkanInstance& vulkanInstance) {
    std::vector<VkDescriptorSetLayoutBinding> floorUniformLayoutBinding;
    floorUniformLayoutBinding.resize(3);

    floorUniformLayoutBinding[0].binding = 0;
    floorUniformLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    floorUniformLayoutBinding[0].descriptorCount = 1;
    floorUniformLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    floorUniformLayoutBinding[0].pImmutableSamplers = nullptr;

    floorUniformLayoutBinding[1].binding = 1;
    floorUniformLayoutBinding[1].descriptorCount = 1;
    floorUniformLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    floorUniformLayoutBinding[1].pImmutableSamplers = nullptr;
    floorUniformLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    floorUniformLayoutBinding[2].binding = 2;
    floorUniformLayoutBinding[2].descriptorCount = 1;
    floorUniformLayoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    floorUniformLayoutBinding[2].pImmutableSamplers = nullptr;
    floorUniformLayoutBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(floorUniformLayoutBinding.size());
    layoutInfo.pBindings = floorUniformLayoutBinding.data();
    if (vkCreateDescriptorSetLayout(vulkanInstance.getLogicalDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create floor descriptor set layout!");
    }
}

void Floor::createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) {
    std::vector<VkDescriptorSetLayout> floorLayouts(count, mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = floorLayouts.data();
    mDescriptorSets.resize(count);
    if(vkAllocateDescriptorSets(vulkanInstance.getLogicalDevice(), &allocInfo, mDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate floor descriptor sets!");
    }

    for (size_t i = 0; i < count; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = uboSize;

        VkDescriptorImageInfo floorImageInfo{};
        floorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        floorImageInfo.imageView = mFloorTextureImageView;
        floorImageInfo.sampler = mFloorTextureSampler;

        VkDescriptorImageInfo shadowMapInfo{};
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = shadowMapImageViews[i];
        shadowMapInfo.sampler = shadowMapSampler;

        std::array<VkWriteDescriptorSet, 3> FloorDescriptorWrites{};
        FloorDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FloorDescriptorWrites[0].dstSet = mDescriptorSets[i];
        FloorDescriptorWrites[0].dstBinding = 0;
        FloorDescriptorWrites[0].dstArrayElement = 0;
        FloorDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        FloorDescriptorWrites[0].descriptorCount = 1;
        FloorDescriptorWrites[0].pBufferInfo = &bufferInfo;
        FloorDescriptorWrites[0].pImageInfo = nullptr; // Optional
        FloorDescriptorWrites[0].pTexelBufferView = nullptr; // Optional

        FloorDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FloorDescriptorWrites[1].dstSet = mDescriptorSets[i];
        FloorDescriptorWrites[1].dstBinding = 1;
        FloorDescriptorWrites[1].dstArrayElement = 0;
        FloorDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        FloorDescriptorWrites[1].descriptorCount = 1;
        FloorDescriptorWrites[1].pImageInfo = &floorImageInfo;

        FloorDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FloorDescriptorWrites[2].dstSet = mDescriptorSets[i];
        FloorDescriptorWrites[2].dstBinding = 2;
        FloorDescriptorWrites[2].dstArrayElement = 0;
        FloorDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        FloorDescriptorWrites[2].descriptorCount = 1;
        FloorDescriptorWrites[2].pImageInfo = &shadowMapInfo; // Optional
        FloorDescriptorWrites[2].pTexelBufferView = nullptr; // Optional
        vkUpdateDescriptorSets(vulkanInstance.getLogicalDevice(), static_cast<uint32_t>(FloorDescriptorWrites.size()),
                FloorDescriptorWrites.data(), 0, nullptr);
    }
}

void Floor::clearResource(VulkanInstance& vulkanInstance) {
    vkDestroyImage(vulkanInstance.getLogicalDevice(), mFloorTextureImage, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mFloorTextureImageMemory, nullptr);
    vkDestroyImageView(vulkanInstance.getLogicalDevice(), mFloorTextureImageView, nullptr);
    vkDestroySampler(vulkanInstance.getLogicalDevice(), mFloorTextureSampler, nullptr);
    Object::clearResource(vulkanInstance);
}

void Floor::createTextures(VulkanInstance& vulkanInstance) {
    createFloorTextureImage(vulkanInstance);
    createFloorTextureImageView(vulkanInstance);
    createFloorTextureSampler(vulkanInstance);
}
