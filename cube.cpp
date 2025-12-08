#include "cube.hpp"
#include "utility.hpp"
#include <stb_image.h>
#include <string.h>

namespace {
    const std::string CUBE_TEXTURE_PATH = "textures/Texturelabs_Wood_134M.jpg";
}

void Cube::initializeObject() {
            // Face 1
        Vertex Face1Vertex1{};
        Face1Vertex1.pos = glm::vec3(1.0, 1.0, 1.0) * 2.0f;
        Face1Vertex1.normal = glm::vec3(0.0, 0.0, 1.0);
        Face1Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face1Vertex1);
        Vertex Face1Vertex2{};
        Face1Vertex2.pos = glm::vec3(1.0, -1.0, 1.0) * 2.0f;
        Face1Vertex2.normal = glm::vec3(0.0, 0.0, 1.0);
        Face1Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face1Vertex2);
        Vertex Face1Vertex3{};
        Face1Vertex3.pos = glm::vec3(-1.0, -1.0, 1.0) * 2.0f;
        Face1Vertex3.normal = glm::vec3(0.0, 0.0, 1.0);
        Face1Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face1Vertex3);
        Vertex Face1Vertex4{};
        Face1Vertex4.pos = glm::vec3(-1.0, 1.0, 1.0) * 2.0f;
        Face1Vertex4.normal = glm::vec3(0.0, 0.0, 1.0);
        Face1Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face1Vertex4);
        mIndices.push_back(0);
        mIndices.push_back(2);
        mIndices.push_back(1);
        mIndices.push_back(0);
        mIndices.push_back(3);
        mIndices.push_back(2);

        // Face 2
        Vertex Face2Vertex1{};
        Face1Vertex1.pos = glm::vec3(1.0, 1.0, -1.0) * 2.0f;
        Face1Vertex1.normal = glm::vec3(1.0, 0.0, 0.0);
        Face1Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face1Vertex1);
        Vertex Face2Vertex2{};
        Face2Vertex2.pos = glm::vec3(1.0, -1.0, -1.0) * 2.0f;
        Face2Vertex2.normal = glm::vec3(1.0, 0.0, 0.0);
        Face2Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face2Vertex2);
        Vertex Face2Vertex3{};
        Face2Vertex3.pos = glm::vec3(1.0, -1.0, 1.0) * 2.0f;
        Face2Vertex3.normal = glm::vec3(1.0, 0.0, 0.0);
        Face2Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face2Vertex3);
        Vertex Face2Vertex4{};
        Face2Vertex4.pos = glm::vec3(1.0, 1.0, 1.0) * 2.0f;
        Face2Vertex4.normal = glm::vec3(1.0, 0.0, 0.0);
        Face2Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face2Vertex4);
        mIndices.push_back(4);
        mIndices.push_back(6);
        mIndices.push_back(5);
        mIndices.push_back(4);
        mIndices.push_back(7);
        mIndices.push_back(6);

        // Face 3
        Vertex Face3Vertex1{};
        Face3Vertex1.pos = glm::vec3(-1.0, 1.0, 1.0) * 2.0f;
        Face3Vertex1.normal = glm::vec3(-1.0, 0.0, 0.0);
        Face3Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face3Vertex1);
        Vertex Face3Vertex2{};
        Face3Vertex2.pos = glm::vec3(-1.0, -1.0, 1.0) * 2.0f;
        Face3Vertex2.normal = glm::vec3(-1.0, 0.0, 0.0);
        Face3Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face3Vertex2);
        Vertex Face3Vertex3{};
        Face3Vertex3.pos = glm::vec3(-1.0, -1.0, -1.0) * 2.0f;
        Face3Vertex3.normal = glm::vec3(-1.0, 0.0, 0.0);
        Face3Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face3Vertex3);
        Vertex Face3Vertex4{};
        Face3Vertex4.pos = glm::vec3(-1.0, 1.0, -1.0) * 2.0f;
        Face3Vertex4.normal = glm::vec3(-1.0, 0.0, 0.0);
        Face3Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face3Vertex4);
        mIndices.push_back(8);
        mIndices.push_back(10);
        mIndices.push_back(9);
        mIndices.push_back(8);
        mIndices.push_back(11);
        mIndices.push_back(10);

        // Face 4
        Vertex Face4Vertex1{};
        Face4Vertex1.pos = glm::vec3(1.0, 1.0, -1.0) * 2.0f;
        Face4Vertex1.normal = glm::vec3(0.0, 1.0, 0.0);
        Face4Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face4Vertex1);
        Vertex Face4Vertex2{};
        Face4Vertex2.pos = glm::vec3(1.0, 1.0, 1.0) * 2.0f;
        Face4Vertex2.normal = glm::vec3(0.0, 1.0, 0.0);
        Face4Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face4Vertex2);
        Vertex Face4Vertex3{};
        Face4Vertex3.pos = glm::vec3(-1.0, 1.0, 1.0) * 2.0f;
        Face4Vertex3.normal = glm::vec3(0.0, 1.0, 0.0);
        Face4Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face4Vertex3);
        Vertex Face4Vertex4{};
        Face4Vertex4.pos = glm::vec3(-1.0, 1.0, -1.0) * 2.0f;
        Face4Vertex4.normal = glm::vec3(0.0, 1.0, 0.0);
        Face4Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face4Vertex4);
        mIndices.push_back(12);
        mIndices.push_back(14);
        mIndices.push_back(13);
        mIndices.push_back(12);
        mIndices.push_back(15);
        mIndices.push_back(14);

        // Face 5
        Vertex Face5Vertex1{};
        Face5Vertex1.pos = glm::vec3(-1.0, 1.0, -1.0) * 2.0f;
        Face5Vertex1.normal = glm::vec3(0.0, 0.0, -1.0);
        Face5Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face5Vertex1);
        Vertex Face5Vertex2{};
        Face5Vertex2.pos = glm::vec3(-1.0, -1.0, -1.0) * 2.0f;
        Face5Vertex2.normal = glm::vec3(0.0, 0.0, -1.0);
        Face5Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face5Vertex2);
        Vertex Face5Vertex3{};
        Face5Vertex3.pos = glm::vec3(1.0, -1.0, -1.0) * 2.0f;
        Face5Vertex3.normal = glm::vec3(0.0, 0.0, -1.0);
        Face5Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face5Vertex3);
        Vertex Face5Vertex4{};
        Face5Vertex4.pos = glm::vec3(1.0, 1.0, -1.0) * 2.0f;
        Face5Vertex4.normal = glm::vec3(0.0, 0.0, -1.0);
        Face5Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face5Vertex4);
        mIndices.push_back(16);
        mIndices.push_back(18);
        mIndices.push_back(17);
        mIndices.push_back(16);
        mIndices.push_back(19);
        mIndices.push_back(18);

        // Face 6
        Vertex Face6Vertex1{};
        Face6Vertex1.pos = glm::vec3(1.0, -1.0, 1.0) * 2.0f;
        Face6Vertex1.normal = glm::vec3(0.0, -1.0, 0.0);
        Face6Vertex1.texCoord = glm::vec2(1.0, 0.0);
        mVertices.push_back(Face6Vertex1);
        Vertex Face6Vertex2{};
        Face6Vertex2.pos = glm::vec3(1.0, -1.0, -1.0) * 2.0f;
        Face6Vertex2.normal = glm::vec3(0.0, -1.0, 0.0);
        Face6Vertex2.texCoord = glm::vec2(1.0, 1.0);
        mVertices.push_back(Face6Vertex2);
        Vertex Face6Vertex3{};
        Face6Vertex3.pos = glm::vec3(-1.0, -1.0, -1.0) * 2.0f;
        Face6Vertex3.normal = glm::vec3(0.0, -1.0, 0.0);
        Face6Vertex3.texCoord = glm::vec2(0.0, 1.0);
        mVertices.push_back(Face6Vertex3);
        Vertex Face6Vertex4{};
        Face6Vertex4.pos = glm::vec3(-1.0, -1.0, 1.0) * 2.0f;
        Face6Vertex4.normal = glm::vec3(0.0, -1.0, 0.0);
        Face6Vertex4.texCoord = glm::vec2(0.0, 0.0);
        mVertices.push_back(Face6Vertex4);
        mIndices.push_back(20);
        mIndices.push_back(22);
        mIndices.push_back(21);
        mIndices.push_back(20);
        mIndices.push_back(23);
        mIndices.push_back(22);
}

void Cube::createVertexBuffer(VulkanInstance& vulkanInstance) {
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

void Cube::createIndexBuffer(VulkanInstance& vulkanInstance) {
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

void Cube::createGraphicsPipeline(VulkanInstance& vulkanInstance,
                                    const VkExtent2D swapChainExtent,
                                    const VkRenderPass renderPass) {
    auto cubeVertShaderCode = readFile("shaders/lambertVert.spv");
    auto cubeFragShaderCode = readFile("shaders/lambertFrag.spv");

    VkShaderModule cubeVertShaderModule = vulkanInstance.createShaderModule(cubeVertShaderCode);
    VkShaderModule cubeFragShaderModule = vulkanInstance.createShaderModule(cubeFragShaderCode);
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = cubeVertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = cubeFragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo cubeShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

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
    pipelineInfo.pStages = cubeShaderStages;
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

    vkDestroyShaderModule(vulkanInstance.getLogicalDevice(), cubeFragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanInstance.getLogicalDevice(), cubeVertShaderModule, nullptr);
}

void Cube::createCubeTextureImage(VulkanInstance& vulkanInstance) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(CUBE_TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mCubeTextureImage, mCubeTextureImageMemory);

    transitionImageLayout(vulkanInstance, mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels);
    copyBufferToImage(vulkanInstance, stagingBuffer, mCubeTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(vulkanInstance, mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mMipLevels);

    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), stagingBufferMemory, nullptr);
}

void Cube::createCubeTextureImageView(VulkanInstance& vulkanInstance) {
    mCubeTextureImageView = createImageView(vulkanInstance.getLogicalDevice(), mCubeTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels);
}

void Cube::createCubeTextureSampler(VulkanInstance& vulkanInstance) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vulkanInstance.getPhysicalDevice(), &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0F;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        if (vkCreateSampler(vulkanInstance.getLogicalDevice(), &samplerInfo, nullptr, &mCubeTextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
}

void Cube::createDescriptorSetLayout(VulkanInstance& vulkanInstance) {
    std::vector<VkDescriptorSetLayoutBinding> cubeUniformBindings;
    cubeUniformBindings.resize(3);

    cubeUniformBindings[0].binding = 0;
    cubeUniformBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cubeUniformBindings[0].descriptorCount = 1;
    cubeUniformBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    cubeUniformBindings[0].pImmutableSamplers = nullptr;

    cubeUniformBindings[1].binding = 1;
    cubeUniformBindings[1].descriptorCount = 1;
    cubeUniformBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubeUniformBindings[1].pImmutableSamplers = nullptr;
    cubeUniformBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    cubeUniformBindings[2].binding = 2;
    cubeUniformBindings[2].descriptorCount = 1;
    cubeUniformBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubeUniformBindings[2].pImmutableSamplers = nullptr;
    cubeUniformBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(cubeUniformBindings.size());
    layoutInfo.pBindings = cubeUniformBindings.data();
    if (vkCreateDescriptorSetLayout(vulkanInstance.getLogicalDevice(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cube descriptor set layout!");
    }
}

void Cube::createDescriptorSets(VulkanInstance& vulkanInstance,
                                const uint8_t count, VkDescriptorPool& descriptorPool,
                                const std::vector<VkBuffer>& uniformBuffers, const VkDeviceSize uboSize,
                                const std::vector<VkImageView>& shadowMapImageViews,
                                const VkSampler& shadowMapSampler) {
    std::vector<VkDescriptorSetLayout> cubeLayouts(count, mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = cubeLayouts.data();
    mDescriptorSets.resize(count);
    if(vkAllocateDescriptorSets(vulkanInstance.getLogicalDevice(), &allocInfo, mDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate cube descriptor sets!");
    }

    for (size_t i = 0; i < count; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = uboSize;

        VkDescriptorImageInfo cubeImageInfo{};
        cubeImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cubeImageInfo.imageView = mCubeTextureImageView;
        cubeImageInfo.sampler = mCubeTextureSampler;

        VkDescriptorImageInfo shadowMapInfo{};
        shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shadowMapInfo.imageView = shadowMapImageViews[i];
        shadowMapInfo.sampler = shadowMapSampler;

        std::array<VkWriteDescriptorSet, 3> cubeDescriptorWrites{};
        cubeDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cubeDescriptorWrites[0].dstSet = mDescriptorSets[i];
        cubeDescriptorWrites[0].dstBinding = 0;
        cubeDescriptorWrites[0].dstArrayElement = 0;
        cubeDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cubeDescriptorWrites[0].descriptorCount = 1;
        cubeDescriptorWrites[0].pBufferInfo = &bufferInfo;
        cubeDescriptorWrites[0].pImageInfo = nullptr; // Optional
        cubeDescriptorWrites[0].pTexelBufferView = nullptr; // Optional

        cubeDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cubeDescriptorWrites[1].dstSet = mDescriptorSets[i];
        cubeDescriptorWrites[1].dstBinding = 1;
        cubeDescriptorWrites[1].dstArrayElement = 0;
        cubeDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubeDescriptorWrites[1].descriptorCount = 1;
        cubeDescriptorWrites[1].pImageInfo = &cubeImageInfo;

        cubeDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cubeDescriptorWrites[2].dstSet = mDescriptorSets[i];
        cubeDescriptorWrites[2].dstBinding = 2;
        cubeDescriptorWrites[2].dstArrayElement = 0;
        cubeDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cubeDescriptorWrites[2].descriptorCount = 1;
        cubeDescriptorWrites[2].pImageInfo = &shadowMapInfo;

        vkUpdateDescriptorSets(vulkanInstance.getLogicalDevice(), static_cast<uint32_t>(cubeDescriptorWrites.size()),
            cubeDescriptorWrites.data(), 0, nullptr);
    }
}

void Cube::clearResource(VulkanInstance& vulkanInstance) {
    vkDestroyImage(vulkanInstance.getLogicalDevice(), mCubeTextureImage, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mCubeTextureImageMemory, nullptr);
    vkDestroyImageView(vulkanInstance.getLogicalDevice(), mCubeTextureImageView, nullptr);
    vkDestroySampler(vulkanInstance.getLogicalDevice(), mCubeTextureSampler, nullptr);
    Object::clearResource(vulkanInstance);
}

void Cube::createTextures(VulkanInstance& vulkanInstance) {
    createCubeTextureImage(vulkanInstance);
    createCubeTextureImageView(vulkanInstance);
    createCubeTextureSampler(vulkanInstance);    
}
