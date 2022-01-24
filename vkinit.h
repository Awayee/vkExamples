#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <vector>
#include <iostream>
#include <fstream>

namespace vkinit {
	std::vector<char> ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule ShaderModule(VkDevice device, const std::string& filename) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		std::vector<char> code = ReadFile(filename);
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shaderModule;
	}

	std::vector<VkPipelineShaderStageCreateInfo> PipelineShaderStageCreateInfo(VkShaderModule vert, VkShaderModule frag) {
		// 创建Shader Stage
		std::vector<VkPipelineShaderStageCreateInfo> shaderInfo{};
		if (vert != VK_NULL_HANDLE) {
			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vert;
			vertShaderStageInfo.pName = "main";
			shaderInfo.push_back(vertShaderStageInfo);
		}
		if (frag != VK_NULL_HANDLE) {
			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = frag;
			fragShaderStageInfo.pName = "main";
			shaderInfo.push_back(fragShaderStageInfo);
		}
		return shaderInfo;
	}


	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(uint32_t bindDesCount, VkVertexInputBindingDescription* pBindDes, uint32_t attributeDesCount, VkVertexInputAttributeDescription* pAttributeDes) {
		//顶点输入
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = bindDesCount;
		vertexInputInfo.pVertexBindingDescriptions = pBindDes;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDesCount;
		vertexInputInfo.pVertexAttributeDescriptions = pAttributeDes;
		return vertexInputInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo() {
		// 输入组件
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		return inputAssembly;
	}

	VkViewport Viewport(float width, float height) {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)width;
		viewport.height = (float)height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		return viewport;
	}

	VkRect2D Rect2D(uint32_t width, uint32_t height) {
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };
		return scissor;
	}

	VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(VkViewport* viewport, VkRect2D* scissor) {
		// 视口和裁剪
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = scissor;
		return viewportState;
	}

	VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo() {
		// 光栅化
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		return rasterizer;
	}


	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo() {
		// 重采样
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional
		return multisampling;
	}


	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState() {
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		return colorBlendAttachment;
	}


	VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments) {
		// 混合

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = attachmentCount;
		colorBlending.pAttachments = pAttachments;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional
		return colorBlending;
	}

	VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo() {
		// 动态调整
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;
		return dynamicState;
	}

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(uint32_t setLayoutCount, VkDescriptorSetLayout* pLayout) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = setLayoutCount; // 
		pipelineLayoutInfo.pSetLayouts = pLayout; // 
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
		return pipelineLayoutInfo;
	}

	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo() {
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;  // 暂不使用深度范围
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;  // 暂不使用模板测试
		depthStencil.front = {};
		depthStencil.back = {};
		return depthStencil;
	}

	VkAttachmentDescription AttachmentDescription(VkImageLayout finalLayout) {
		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_D32_SFLOAT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = finalLayout;
		attachment.flags = 0;
		return attachment;
	}

	VkAttachmentReference AttachmentReference(uint32_t attachment, VkImageLayout layout) {
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = attachment;
		depthAttachmentRef.layout = layout;
		return depthAttachmentRef;
	}

	VkSubpassDescription SubpassDescription(uint32_t colorAttachmentCount, VkAttachmentReference* pColorAttachmentRef, VkAttachmentReference* pDepthRef) {
		VkSubpassDescription subpass;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = colorAttachmentCount;
		subpass.pColorAttachments = pColorAttachmentRef;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = pDepthRef;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;
		return subpass;
	}

	VkPushConstantRange PushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlagBits shaderStage) {
		VkPushConstantRange constantRange{};
		constantRange.offset = offset;
		constantRange.size = size;
		constantRange.stageFlags = shaderStage;
		return constantRange;
	}

	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlagBits stage) {
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = binding;
		layoutBinding.descriptorCount = 1;
		layoutBinding.descriptorType = type;
		layoutBinding.stageFlags = stage;
		layoutBinding.pImmutableSamplers = nullptr;
		return layoutBinding;
	}
}