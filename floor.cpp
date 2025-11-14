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

void Floor::createGraphicsPipeline(VulkanInstance& vulkanInstance,
                                    const VkExtent2D swapChainExtent,
                                    const VkRenderPass renderPass,
                                    const VkDescriptorSetLayout* descriptorSetLayout) {
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
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
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

void Floor::clearResource(VulkanInstance& vulkanInstance) {
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mVertexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanInstance.getLogicalDevice(), mIndexBuffer, nullptr);
    vkFreeMemory(vulkanInstance.getLogicalDevice(), mIndexBufferMemory, nullptr);
    vkDestroyPipelineLayout(vulkanInstance.getLogicalDevice(), mPipelineLayout, nullptr);
    vkDestroyPipeline(vulkanInstance.getLogicalDevice(), mGraphicsPipeline, nullptr);
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

const VkPipeline Floor::getGraphicsPipeline() const {
    return mGraphicsPipeline;
}

const VkPipelineLayout Floor::getPipelineLayout() const {
    return mPipelineLayout;
}
