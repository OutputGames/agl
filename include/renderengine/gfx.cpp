#include "gfx.hpp"

#include "application.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

ShaderLevel::ShaderLevel(string code, Application* app, ShaderType type, Shader* parent)
{

	application = app;
	this->parent = parent;

	VkShaderModuleCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const u32*>(code.data());

	VkResult result = vkCreateShaderModule(app->GetDevice(), &createInfo, nullptr, &module);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module.");
	}

	SpvReflectShaderModule reflectModule;

	SpvReflectResult reflectResult = spvReflectCreateShaderModule(code.size(), code.data(), &reflectModule);

	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);

	u32 descriptorSet_count = 0;

	spvReflectEnumerateDescriptorSets(&reflectModule, &descriptorSet_count, NULL);

	SpvReflectDescriptorSet** descriptor_sets = (SpvReflectDescriptorSet**)malloc(descriptorSet_count * sizeof(SpvReflectDescriptorSet*));

	spvReflectEnumerateDescriptorSets(&reflectModule, &descriptorSet_count, descriptor_sets);

	for (int i = 0; i < descriptorSet_count; ++i)
	{
		SpvReflectDescriptorSet* input_var = descriptor_sets[i];

		for (int j = 0; j < input_var->binding_count; ++j)
		{
			SpvReflectDescriptorBinding* binding = input_var->bindings[j];

			DescriptorPort* port = new DescriptorPort;

			if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				TexturePort* tport = new TexturePort;
				tport->texture = nullptr;
				port = tport;
			}

			port->type = static_cast<DescriptorType>(binding->descriptor_type);
			port->name = binding->name;
			port->binding = binding->binding;
			port->set = binding->set;
			port->count = binding->count;
			
			parent->ports.push_back(port);

			cout << "Descriptor binding found: " << binding->name << " found at " << binding->binding << endl;
		}

			
	}

	spvReflectDestroyShaderModule(&reflectModule);

	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = (VkShaderStageFlagBits)(type);

	shaderStageInfo.module = module;
	shaderStageInfo.pName = "main";

	stageInfo = shaderStageInfo;

}

void ShaderLevel::Destroy()
{
	vkDestroyShaderModule(application->GetDevice(), module, nullptr);
}

CommandBuffer::CommandBuffer(Application* app)
{
	application = app;

	// Command Pool
	CreateCommandPool();

}

void CommandBuffer::Begin(u32 currentImage)
{

	vkResetCommandBuffer(commandBuffers[currentImage], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffers[currentImage], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer.");
	}
}

void CommandBuffer::End(u32 currentImage)
{

	vkCmdEndRenderPass(commandBuffers[currentImage]);
	VkResult result = vkEndCommandBuffer(commandBuffers[currentImage]);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer.");
	}
}

VkCommandBuffer CommandBuffer::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer vkCommandBuffer;
	vkAllocateCommandBuffers(agl::device, &allocInfo, &vkCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);

	return vkCommandBuffer;
}

void CommandBuffer::EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer)
{
	vkEndCommandBuffer(vkCommandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;

	vkQueueSubmit(agl::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(agl::graphicsQueue);

	vkFreeCommandBuffers(agl::device, commandPool, 1, &vkCommandBuffer);
}

void CommandBuffer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = application->FindQueueFamilies(application->physicalDevice);

	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(application->GetDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool.");
	}
}

void CommandBuffer::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<u32>(commandBuffers.size());

	if (vkAllocateCommandBuffers(application->GetDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers.");
	}
}


RenderPass::RenderPass(Application* app, Framebuffer* framebuffer)
{
	application = app;

	this->framebuffer = framebuffer;

	CreateRenderPass();
}

void RenderPass::AttachToCommandBuffer(CommandBuffer* buffer)
{
	commandBuffer = buffer;
}

void RenderPass::Begin(u32 imageIndex)
{
	VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer->swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = framebuffer->swapChainExtent;

	array<VkClearValue, 2> clearValues{};

	clearValues[0].color = { 0.0f,0.0f,0.0f,1.0f };
	clearValues[1].depthStencil = { 1.0f,0 };

	renderPassInfo.clearValueCount = cast(clearValues.size(), u32);
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer->GetCommandBuffer(imageIndex), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(framebuffer->swapChainExtent.width);
	viewport.height = static_cast<float>(framebuffer->swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer->GetCommandBuffer(imageIndex), 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = framebuffer->swapChainExtent;
	vkCmdSetScissor(commandBuffer->GetCommandBuffer(imageIndex), 0, 1, &scissor);
}

void RenderPass::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = framebuffer->swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = application->FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{ 1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference colorAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;


	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = cast(attachments.size(),u32);
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(application->GetDevice(), &renderPassInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass.");
	}
}


Framebuffer::Framebuffer(Application* app)
{
	application = app;

	CreateSwapChain();
	CreateDepthResources();
	CreateImageViews();

	renderPass = new RenderPass(app, this);
}

void Framebuffer::Destroy()
{
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(application->device, imageView, nullptr);
	}

	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(application->device, framebuffer, nullptr);
	}

	vkDestroySwapchainKHR(application->device, swapChain, nullptr);
}

void Framebuffer::Recreate()
{

	int width = 0, height = 0;
	glfwGetFramebufferSize(application->window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(application->window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(application->device);

	Destroy();

	CreateSwapChain();
	CreateDepthResources();
	CreateImageViews();
	CreateFramebuffers();
}

void Framebuffer::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = application->querySwapChainSupport(application->physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = application->chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = application->chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = application->ChooseSwapExtent(swapChainSupport.capabilities);

	u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = application->surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = application->FindQueueFamilies(application->physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(application->GetDevice(), &createInfo, nullptr, &swapChain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain.");
	}

	vkGetSwapchainImagesKHR(application->GetDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(application->GetDevice(), swapChain, &imageCount, swapChainImages.data());

	swapChainExtent = extent;
	swapChainImageFormat = surfaceFormat.format;
}

void Framebuffer::CreateImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (int i = 0; i < swapChainImages.size(); ++i)
	{
		swapChainImageViews[i] = Texture::CreateImageView(swapChainImages[i], swapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT, application->device);
	}
}

void Framebuffer::CreateDepthResources()
{
	VkFormat depthFormat = application->FindDepthFormat();

	Texture::CreateVulkanImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, application->device, application->physicalDevice);

	depthImageView = Texture::CreateImageView(depthImage, depthFormat,VK_IMAGE_ASPECT_DEPTH_BIT, application->device);
	//application->TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void Framebuffer::CreateFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		array<VkImageView,2> attachments = {
			swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass->renderPass;
		framebufferInfo.attachmentCount = cast(attachments.size(),u32);
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(application->GetDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer.");
		}
	}
}


void Shader::CreateDescriptorSetLayout()
{

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<u32>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(application->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
}

void Shader::CreateDescriptorPool()
{
	VkDescriptorPoolCreateInfo poolInfo{};

	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(application->GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool.");
	}
}

void Shader::CreateDescriptorSet()
{


	for (auto port : ports)
	{
		if (port->type == DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			TexturePort* tport = static_cast<TexturePort*>(port);
			Texture* texture = tport->texture;
			if (texture) {
				for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
				{

					VkImageView* iv = new VkImageView(texture->textureImageView);
					VkSampler* sampler = new VkSampler(texture->textureSampler);

					VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
					imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo->imageView = *iv;
					imageInfo->sampler = *sampler;

					VkWriteDescriptorSet* descriptorWrite;

					descriptorWrite = CreateDescriptorSetWrite(i);
					descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite->pImageInfo = imageInfo;

					AttachDescriptorWrite(descriptorWrite, i);
				}
			}
		}
	}

	vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, GetDescriptorSetLayout());
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = GetDescriptorPool();
	allocInfo.descriptorSetCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(application->GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets.");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vector<VkWriteDescriptorSet> writes(descriptorWrites[i].size());
		int ctr = 0;
		for (VkWriteDescriptorSet descriptor_write : descriptorWrites[i])
		{
			descriptor_write.dstSet = descriptorSets[i];


			writes[ctr] = descriptor_write;
			ctr++;
		}

		vkUpdateDescriptorSets(application->GetDevice(), static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
	}

}

void Shader::CreateGraphicsPipeline()
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	auto bindingDesc = Vertex::GetBindingDescription();
	auto attributeDesc = Vertex::GetAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDesc.size());

	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = settings->cullFlags;
	rasterizer.frontFace = settings->frontFace;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

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

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = settings->depthCompare;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.f;
	depthStencil.stencilTestEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(application->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = application->framebuffer->GetRenderPass()->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(application->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline.");
	}
}

void Shader::Setup()
{
	// Descriptor Set Layout

	CreateDescriptorSetLayout();

	CreateDescriptorPool();

	// Graphics Pipeline

	CreateGraphicsPipeline();

}

Shader::Shader(ShaderSettings* settings, Application* appl)
{

	descriptorWrites.resize(MAX_FRAMES_IN_FLIGHT);
	this->settings = settings;

	vertexCode = ReadString(settings->vertexPath);
	vertModule = new ShaderLevel(vertexCode, appl, VERTEX, this);

	fragmentCode = ReadString(settings->fragmentPath);
	fragModule = new ShaderLevel(fragmentCode, appl, FRAGMENT, this);

	application = appl;

	shaderStages = { vertModule->stageInfo, fragModule->stageInfo };
}

u32 Shader::GetBindingByName(string n)
{

	for (DescriptorPort* port : ports)
	{
		if (port->name == n)
		{
			return port->binding;
		}
	}

	return -1;
}

void Shader::Destroy()
{
	fragModule->Destroy();
	vertModule->Destroy();

	vkDestroyDescriptorSetLayout(application->GetDevice(), descriptorSetLayout, nullptr);
	vkDestroyPipeline(application->GetDevice(), graphicsPipeline, nullptr);
}

void Shader::BindGraphicsPipeline(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Shader::BindDescriptor(VkCommandBuffer commandBuffer, u32 currentImage)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetPipelineLayout(), 0, 1, &descriptorSets[currentImage], 0, nullptr);
}

VkPipelineLayout Shader::GetPipelineLayout()
{
	return pipelineLayout;
}

void Shader::AttachTexture(Texture* texture, u32 binding)
{

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = poolSizes.size();
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkDescriptorPoolSize samplerPoolSize{};

	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

	AttachDescriptorPool(samplerPoolSize);
	AttachDescriptorSetLayout(samplerLayoutBinding);

	if (binding != -1)
	{

		TexturePort* port = static_cast<TexturePort*>(ports[binding]);

		port->texture = texture;
	}


}

void Camera::Update(VkExtent2D extent)
{
	vec3 direction = normalize(position - target);

	right = normalize(cross(up, direction));
	up = cross(direction, right);

	view = lookAt(position, target, {0,1,0});

	proj = glm::perspective(glm::radians(fov), flt extent.width / flt extent.height, 0.1f, 100.0f);

}

Texture::Texture(Application* app, string path)
{
	application = app;
	this->path = path;
	int texWidth, texHeight, texChannels;

	stbi_set_flip_vertically_on_load(true);

	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	width = texWidth;
	height = texHeight;
	channels = texChannels;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	application->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(app->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(app->device, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateVulkanImage(width,height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, app->device, app->physicalDevice);

	app->TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	app->CopyBufferToImage(stagingBuffer, textureImage, static_cast<u32>(texWidth), static_cast<u32>(texHeight));
	app->TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	app->vkDestroy(vkDestroyBuffer, stagingBuffer);
	app->vkDestroy(vkFreeMemory, stagingBufferMemory);

	 textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_ASPECT_COLOR_BIT, app->device);
	CreateTextureSampler();

}



VkImageView Texture::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { aspectFlags, 0,1,0,1 };

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image view");
	}

	return imageView;
}

void Texture::CreateVulkanImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice device, VkPhysicalDevice physicalDevice)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Application::FindMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}


void Texture::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};

	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(application->physicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(application->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

Mesh::Mesh(aiMesh* mesh, Application* app)
{
	application = app;
	 
	for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
	{
		aiVector3D v = mesh->mVertices[j];
		aiVector3D t = mesh->mTextureCoords[0][j];
		aiVector3D n = mesh->mNormals[j];

		Vertex vtx = { {v.x, v.y,v.z}, {n.x,n.y,n.z}, {t.x, t.y} };

		vertices.push_back(vtx);
	}
	for (unsigned int j = 0; j < mesh->mNumFaces; j++)
	{
		aiFace face = mesh->mFaces[j];
		for (unsigned int l = 0; l < face.mNumIndices; l++)
			indices.push_back(face.mIndices[l]);
	}


	{

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		app->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(app->device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(app->device, stagingBufferMemory);

		app->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		app->CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(app->device, stagingBuffer, nullptr);
		vkFreeMemory(app->device, stagingBufferMemory, nullptr);
	}

	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		app->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(app->device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(app->device, stagingBufferMemory);

		app->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		app->CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(app->device, stagingBuffer, nullptr);
		vkFreeMemory(app->device, stagingBufferMemory, nullptr);
	}

	materialIndex = mesh->mMaterialIndex;

}


void Mesh::Draw(CommandBuffer* commandBuffer, u32 imageIndex)
{

	if (shader) {
		shader->BindGraphicsPipeline(commandBuffer->GetCommandBuffer(application->currentFrame));
	}

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer->GetCommandBuffer(imageIndex), 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer->GetCommandBuffer(imageIndex), indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	if (shader) {
		shader->BindDescriptor(commandBuffer->GetCommandBuffer(imageIndex), imageIndex);
	}

	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer->GetCommandBuffer(imageIndex), static_cast<u32>(indices.size()), 1, 0, 0, 0);
}

Model::Model(string path, Application* app)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	// If the import failed, report it
	if (nullptr == scene) {
		cout << (importer.GetErrorString());
	}

	for (int j = 0; j < scene->mNumTextures; ++j)
	{
		aiTexture* tex = scene->mTextures[j];
		cout << tex->mFilename.C_Str() << endl;

	}

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* aimesh = scene->mMeshes[i];

		Mesh* mesh = new Mesh(aimesh, app);

		meshes.push_back(mesh);
	}

	for (int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* aiMat = scene->mMaterials[i];

		vector<TextureRef> diffuseTextures = LoadMaterialTextures(aiMat, aiTextureType_DIFFUSE, path);
		vector<TextureRef> normalTextures = LoadMaterialTextures(aiMat, aiTextureType_NORMALS, path);

		Material* material = new Material;

		material->textures.insert({ Material::ALBEDO, diffuseTextures });
		material->textures.insert({ Material::NORMAL, normalTextures });

		materials.push_back(material);
	}

	application = app;
}

void Model::Draw(CommandBuffer* commandBuffer, u32 imageIndex)
{
	for (Mesh* mesh : meshes)
	{
		mesh->Draw(commandBuffer, imageIndex);
	}
}

void Model::SetShader(Shader* shader)
{
	for (Mesh* mesh : meshes)
	{
		mesh->shader = shader;
	}
}

vector<TextureRef> Model::LoadMaterialTextures(aiMaterial* material, aiTextureType type, string modelPath)
{
	vector<TextureRef> textures;

	for (int i = 0; i < material->GetTextureCount(type); ++i)
	{
		aiString str;
		material->GetTexture(type, i, &str);

		string directory;
		string path;

		const szt last_slash_idx = modelPath.rfind('\\');
		if (string::npos != last_slash_idx)
		{
			directory = modelPath.substr(0, last_slash_idx);
		}

		filesystem::path spath(str.C_Str());

		string spaths{ spath.filename().u8string() };

		path = directory + "//" + spaths;

		TextureRef ref = { path };

		textures.push_back(ref);
			
	}

	return textures;
}
