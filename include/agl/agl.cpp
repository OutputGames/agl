#include "agl.hpp"

#include "re.hpp"
#include "aurora/utils/fs.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "agl_ext.hpp"

using namespace std;


  bool agl::CheckValidationLayerSupport()
{
	uint32_t layerCt;

	vkEnumerateInstanceLayerProperties(&layerCt, nullptr);

	vector<VkLayerProperties> availableLayers(layerCt);
	vkEnumerateInstanceLayerProperties(&layerCt, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

vector<const char*> agl::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &glfwExtensionCount, nullptr);
	std::vector<const char*> extensions;
	extensions.resize(glfwExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &glfwExtensionCount, extensions.data());

	uint32_t extCt = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCt, nullptr);

	vector<VkExtensionProperties> exts(extCt);

	vkEnumerateInstanceExtensionProperties(nullptr, &extCt, exts.data());

	cout << "Available Extensions: " << endl;

	for (const auto& ext : exts)
	{
		cout << "\t" << ext.extensionName << "\n";
	}

	if (validationLayersEnabled)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}


	cout << "Required extensions: " << endl;

	for (auto extension : extensions)
	{
		cout << "\t" << extension << endl;
	}

	return extensions;
}

VkBool32 agl::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void agl::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

u32 agl::SurfaceDetails::GetNextImageIndex()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	u32 imageIndex;
	VkResult resultkhr = vkAcquireNextImageKHR(device, baseSurface->swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
		&imageIndex);

	if (resultkhr == VK_ERROR_OUT_OF_DATE_KHR) {
		baseSurface->Recreate();
		return 0;
	}
	else if (resultkhr != VK_SUCCESS && resultkhr != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}


	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	currentImage = imageIndex;

	return imageIndex;
}

void agl::SurfaceDetails::Recreate()
{
	Destroy();
	Create();
}

agl::SurfaceDetails::SurfaceDetails()
{
	Create();
}

void agl::SurfaceDetails::Destroy()
{
	commandBuffer->Destroy();
	swapchain->Destroy();
	framebuffer = nullptr;
}

void agl::SurfaceDetails::Create()
{

	swapchain = new aglSwapchain;

	framebuffer = swapchain->fbo;

	commandBuffer = new aglCommandBuffer;

	framebuffer->renderPass->commandBuffer = commandBuffer;

	// General resource creation


	commandBuffer->CreateMainBuffers();

	framebuffer->GetRenderPass()->AttachToCommandBuffer(commandBuffer);

}

void agl::CreateInstance()
{
	if (validationLayersEnabled && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available.");
	}

	VkApplicationInfo application_info{};

	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = details->applicationName.c_str();
	application_info.applicationVersion = details->applicationVersion;
	application_info.pEngineName = details->engineName.c_str();
	application_info.engineVersion = details->engineVersion;
	application_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &application_info;

	auto extensions = GetRequiredExtensions();

	create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (validationLayersEnabled)
	{
		create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		create_info.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		create_info.pNext = &debugCreateInfo;
	}
	else
	{
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}


	VkResult result = vkCreateInstance(&create_info, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance.");
	}
}

void agl::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                       VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void agl::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = baseSurface->commandBuffer->BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	baseSurface->commandBuffer->EndSingleTimeCommands(commandBuffer);
}

void agl::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int layerCount, bool endCmd)
{
	VkCommandBuffer vkCommandBuffer = baseSurface->commandBuffer->BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 };
	barrier.subresourceRange.layerCount = layerCount;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(vkCommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	if (endCmd)
		baseSurface->commandBuffer->EndSingleTimeCommands(vkCommandBuffer);
}

void agl::CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height, u32 regionCount, VkBufferImageCopy* regions)
{
	VkCommandBuffer vkCommandBuffer = baseSurface->commandBuffer->BeginSingleTimeCommands();

	VkBufferImageCopy* bfrs = regions;
	u32 regCt = regionCount;

	if (regionCount == 0) {

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		VkBufferImageCopy dreg[1];

		dreg[0] = region;

		bfrs = dreg;
		regCt = 1;

	}

	vkCmdCopyBufferToImage(vkCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regCt, bfrs);

	baseSurface->commandBuffer->EndSingleTimeCommands(vkCommandBuffer);
}

void agl::CopyImageToImage(VkImage base, VkImage sub, int layer, int layerCount, int width, int height, bool endCmd)
{

	VkCommandBuffer cmdBuf = baseSurface->commandBuffer->BeginSingleTimeCommands();

	// Copy region for transfer from framebuffer to cube face
	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = layerCount;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = layer;
	copyRegion.dstSubresource.mipLevel = 0;
	copyRegion.dstSubresource.layerCount = layerCount;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = static_cast<uint32_t>(width);
	copyRegion.extent.height = static_cast<uint32_t>(height);
	copyRegion.extent.depth = 1;

	vkCmdCopyImage(
		cmdBuf,
		sub,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		base,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copyRegion);

	if (endCmd)
		baseSurface->commandBuffer->EndSingleTimeCommands(cmdBuf);
}

VkFormat agl::FindSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling,
                                  VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format");
}

VkFormat agl::FindDepthFormat()
{
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool agl::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void agl::PresentFrame(u32 imageIndex)
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;


	submitInfo.pCommandBuffers = &baseSurface->commandBuffer->commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult queueResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

	if (queueResult != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { baseSurface->swapchain->swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;


	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || baseSurface->framebuffer->IsResized()) {
		baseSurface->framebuffer->Resized = false;
		baseSurface->Recreate();

		cout << "Recreated Swapchain" << endl;

	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkDeviceWaitIdle(device);
}

void agl::CreateSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void agl::FramebufferResizeCallback(SDL_Window* window, int width, int height)
{
	baseSurface->framebuffer->Resized = true;
}

uint32_t agl::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


void agl::SetupDebugMessenger()
{
	if (!validationLayersEnabled) return;

	VkDebugUtilsMessengerCreateInfoEXT create_info{};
	PopulateDebugMessengerCreateInfo(create_info);

	VkResult result = CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &DebugMessenger);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void agl::PickPhysicalDevice()
{
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support.");
	}

	vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (VkPhysicalDevice device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			physicalDevice = device;

			//std::cout << "Physical Device Selected: " << physical_device. << endl ;

			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU.");
	}
}

bool agl::IsDeviceSuitable(const VkPhysicalDevice device)
{
	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);


	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool agl::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

agl::QueueFamilyIndices agl::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

void agl::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures device_features{};

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	create_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	create_info.pQueueCreateInfos = queueCreateInfos.data();

	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
	create_info.ppEnabledExtensionNames = deviceExtensions.data();

	if (validationLayersEnabled)
	{
		create_info.enabledLayerCount = static_cast<u32>(validationLayers.size());
		create_info.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(physicalDevice, &create_info, nullptr, &device);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device.");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void agl::CreateSurface()
{
	if (SDL_Vulkan_CreateSurface(window, instance, &surface) == 0)
	{
		cout << SDL_GetError() << endl;
		throw std::runtime_error("Failed to create window surface!");
	}
}

agl::SwapChainSupportDetails agl::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	u32 formatCount;
	u32 presentModeCount;

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR agl::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (VkSurfaceFormatKHR availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR agl::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D agl::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
	{
		return capabilities.currentExtent;
	}
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height);

	return actualExtent;
}

void agl::record_command_buffer(u32 imageIndex)
{

	baseSurface->commandBuffer->Begin(imageIndex);

	baseSurface->framebuffer->Bind(imageIndex,baseSurface->commandBuffer->GetCommandBuffer(imageIndex));

	baseSurface->framebuffer->renderPass->PushRenderQueue();
}

void agl::FinishRecordingCommandBuffer(u32 imageIndex)
{
	baseSurface->commandBuffer->End(imageIndex);
}

void agl::DrawFrame()
{
	u32 imageIndex = baseSurface->GetNextImageIndex();

	PresentFrame(imageIndex);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void agl::PollEvent(SDL_Event event)
{
	if (event.type == SDL_QUIT)
		closeWindow = true;
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(agl::window))
		closeWindow = true;
}


agl::aglShaderLevel::aglShaderLevel(string code, aglShaderType type, aglShader* parent)
{
	this->parent = parent;

	VkShaderModuleCreateInfo createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const u32*>(code.data());

	VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);

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

			aglDescriptorPort* port = new aglDescriptorPort;

			if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				aglTexturePort* tport = new aglTexturePort;
				tport->texture = nullptr;
				port = tport;
			}

			port->type = static_cast<aglDescriptorType>(binding->descriptor_type);
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

void agl::aglShaderLevel::Destroy()
{
	vkDestroyShaderModule(device, module, nullptr);
}

agl::aglCommandBuffer::aglCommandBuffer()
{
	// Command Pool
	CreateCommandPool();
}

void agl::aglCommandBuffer::Begin(u32 currentImage)
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

void agl::aglCommandBuffer::End(u32 currentImage)
{
	VkResult result = vkEndCommandBuffer(commandBuffers[currentImage]);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer.");
	}
}

VkCommandBuffer agl::aglCommandBuffer::BeginSingleTimeCommands()
{
	if (currentBufferUsed == VK_NULL_HANDLE) {
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

		currentBufferUsed = vkCommandBuffer;

		return vkCommandBuffer;
	} else
	{
		return currentBufferUsed;
	}
}

void agl::aglCommandBuffer::EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer)
{
	vkEndCommandBuffer(vkCommandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;

	vkQueueSubmit(agl::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(agl::graphicsQueue);

	vkFreeCommandBuffers(agl::device, commandPool, 1, &vkCommandBuffer);

	currentBufferUsed = VK_NULL_HANDLE;

}

void agl::aglCommandBuffer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool.");
	}
}

void agl::aglCommandBuffer::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<u32>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers.");
	}
}

void agl::aglCommandBuffer::Destroy()
{
	vkDestroyCommandPool(device, commandPool, nullptr);
}

agl::aglRenderQueue::aglRenderQueue(aglRenderPass* pass)
{
	this->pass = pass;
}

void agl::aglRenderQueue::Push()
{
	for (aglRenderQueueEntry queue_entry : queueEntries)
	{
		queue_entry.shader->BindGraphicsPipeline(pass->commandBuffer->GetCommandBuffer(agl::currentFrame));
		queue_entry.mesh->Draw(pass->commandBuffer->GetCommandBuffer(agl::currentFrame), agl::currentFrame);
	}

	queueEntries.clear();
}

agl::aglRenderPass::aglRenderPass(aglFramebuffer* framebuffer, aglRenderPassSettings settings)
{
	this->framebuffer = framebuffer;
	this->renderQueue = new aglRenderQueue(this);

	CreateRenderPass(settings);
}

void agl::aglRenderPass::AttachToCommandBuffer(aglCommandBuffer* buffer)
{
	commandBuffer = buffer;
}

void agl::aglRenderPass::Begin(u32 imageIndex, VkCommandBuffer cmdBuf)
{
	VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = framebuffer->extent;

	array<VkClearValue, 2> clearValues{};

	clearValues[0].color = { 0.2f,0.3f,0.3f,1.0f };
	clearValues[1].depthStencil = { 1.0f,0 };

	renderPassInfo.clearValueCount = cast(clearValues.size(), u32);
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(framebuffer->extent.width);
	viewport.height = static_cast<float>(framebuffer->extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = framebuffer->extent;
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

void agl::aglRenderPass::PushRenderQueue()
{
	renderQueue->Push();
}

void agl::aglRenderPass::End(VkCommandBuffer cmdBuf)
{
	vkCmdEndRenderPass(cmdBuf);
}

void agl::aglRenderPass::CreateRenderPass(aglRenderPassSettings settings)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = framebuffer->imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{ 1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference colorAttachmentRef{ 0, settings.colorAttachmentLayout };
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
	renderPassInfo.attachmentCount = cast(attachments.size(), u32);
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(GetDevice(), &renderPassInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass.");
	}
}

agl::aglSwapchain::aglSwapchain()
{
	fbo = new aglFramebuffer();
	CreateSwapChain();
	fbo->CreateFramebuffers();
}

void agl::aglSwapchain::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};

	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[ ] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

	VkResult result = vkCreateSwapchainKHR(GetDevice(), &createInfo, nullptr, &swapChain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain.");
	}

	vkGetSwapchainImagesKHR(GetDevice(), swapChain, &imageCount, nullptr);
	fbo->images.resize(imageCount);
	vkGetSwapchainImagesKHR(GetDevice(), swapChain, &imageCount, fbo->images.data());

	fbo->extent = extent;
	fbo->imageFormat = surfaceFormat.format;
}

void agl::aglSwapchain::Destroy()
{
	fbo->Destroy();
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void agl::aglSwapchain::Recreate()
{
	Destroy();
	CreateSwapChain();
	fbo->Recreate();
}

agl::aglFramebuffer::aglFramebuffer()
{

}

agl::aglFramebuffer::aglFramebuffer(int width, int height, aglFramebufferCreationSettings settings)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkImage image;
		VkDeviceMemory memory;
		aglTexture::CreateVulkanImage(width, height, settings.format, VK_IMAGE_TILING_OPTIMAL, settings.usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory,false);
		images.push_back(image);
	}
	imageFormat = settings.format;
}

void agl::aglFramebuffer::Destroy()
{

	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (auto image_view : imageViews)
	{
		vkDestroyImageView(device, image_view, nullptr);
	}

}

void agl::aglFramebuffer::Recreate()
{
	int width = 0, height = 0;
	SDL_GetWindowSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		SDL_GetWindowSize(window, &width, &height);
		SDL_WaitEvent(event);
	}

	vkDeviceWaitIdle(device);

	Destroy();

	CreateFramebuffers();
}

void agl::aglFramebuffer::CreateFramebuffers()
{
	CreateDepthResources();
	CreateImageViews();

	renderPass = new aglRenderPass(this, aglRenderPassSettings{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
	framebuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		array<VkImageView, 2> attachments = {
			imageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass->renderPass;
		framebufferInfo.attachmentCount = cast(attachments.size(), u32);
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(GetDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer.");
		}
	}
}

void agl::aglFramebuffer::CreateImageViews()
{
	imageViews.resize(images.size());

	for (int i = 0; i < images.size(); ++i)
	{
		imageViews[i] = aglTexture::CreateImageView(images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, false);
	}
}

void agl::aglFramebuffer::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	aglTexture::CreateVulkanImage(extent.width, extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, false);

	depthImageView = aglTexture::CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, false);
	//TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void agl::aglShaderFactory::ReloadAllShaders()
{
	for (auto loaded_shader : loadedShaders)
	{
		loaded_shader->Recreate();
	}
}

void agl::aglShaderFactory::ReloadShader(u32 id)
{
	loadedShaders[id]->Recreate();
}

void agl::aglShaderFactory::InsertShader(aglShader* shader)
{
	loadedShaders.push_back(shader);
}

void agl::aglShader::Setup()
{

	ppBuffer = new aglUniformBuffer<PostProcessingSettings>(this, { VK_SHADER_STAGE_FRAGMENT_BIT });

	if (GetBindingByName("postProcessingSettings") != -1) {
		ppBuffer->AttachToShader(this, GetBindingByName("postProcessingSettings"));
	}

	CreateDescriptorSetLayout();

	CreateDescriptorPool();

	// Graphics Pipeline

	CreateGraphicsPipeline();

	CreateDescriptorSet();

	setup = true;
}

void agl::aglShader::Create()
{

	vertexCode = ReadString(settings.vertexPath);

	if (ustring::hasEnding(settings.vertexPath, "vert"))
	{
		vertexCode = CompileGLSLToSpirV(settings.vertexPath);
	}


	vertModule = new aglShaderLevel(vertexCode, VERTEX, this);

	fragmentCode = ReadString(settings.fragmentPath);

	if (ustring::hasEnding(settings.fragmentPath, "frag"))
	{
		fragmentCode = CompileGLSLToSpirV(settings.fragmentPath);
	}

	fragModule = new aglShaderLevel(fragmentCode, FRAGMENT, this);

	for (int i = 0; i < descriptorWrites.size(); ++i)
	{
		if (descriptorWrites[i].size() != ports.size()) {
			descriptorWrites[i].resize(ports.size());
		}
	}

	shaderStages = { vertModule->stageInfo, fragModule->stageInfo };
}

agl::aglShader::aglShader(aglShaderSettings settings)
{
	descriptorWrites.resize(MAX_FRAMES_IN_FLIGHT);
	this->settings = settings;

	aglShaderFactory::InsertShader(this);

	Create();
}

std::string agl::aglShader::CompileGLSLToSpirV(std::string path)
{

	static vector<string> compiledShaders;

	string elevPath = ustring::substring(path, "resources/shaders/");

	if (ustring::hasEnding(elevPath, ".vert")) {
		elevPath = ustring::substring(elevPath, ".vert");
		elevPath += ".vert-spv";
	}

	if (ustring::hasEnding(elevPath, ".frag")) {
		elevPath = ustring::substring(elevPath, ".frag");
		elevPath += ".frag-spv";
	}

	string spvPath = "compiled/shaders/" + elevPath;

	if (std::find(compiledShaders.begin(), compiledShaders.end(), spvPath) == compiledShaders.end()) {

		compiledShaders.push_back(spvPath);

		filesystem::remove(spvPath);

		filesystem::create_directory("compiled/");
		filesystem::create_directory("compiled/shaders/");

		filesystem::path fpath(spvPath);

		filesystem::path parent = fpath.parent_path();

		filesystem::create_directory(fpath.parent_path());

		string sysCmd = "cd " + filesystem::current_path().string() + " && ";

		sysCmd += "C:/VulkanSDK/1.3.250.0/Bin/glslc ";

		sysCmd += path;

		sysCmd += " -o ";

		sysCmd += spvPath;

		int retcode = system(sysCmd.c_str());

		cout << "Compiled shader: " << spvPath << " Returned: " << retcode << endl;

		if (retcode != 0) {
			return "";
		}

	}

	return ReadString(spvPath);
}

u32 agl::aglShader::GetBindingByName(string n)
{
	for (aglDescriptorPort* port : ports)
	{
		if (port->name == n)
		{
			return port->binding;
		}
	}

	//throw new std::exception(("No binding found with name: " + n).c_str());

	return -1;
}

void agl::aglShader::Destroy()
{
	fragModule->Destroy();
	vertModule->Destroy();

	vkDestroyDescriptorSetLayout(GetDevice(), descriptorSetLayout, nullptr);
	vkDestroyPipeline(GetDevice(), graphicsPipeline, nullptr);
}

void agl::aglShader::Recreate()
{
	Destroy();
	Create();
	Setup();
}

VkWriteDescriptorSet* agl::aglShader::CreateDescriptorSetWrite(int frame, int binding)
{
	auto write = new VkWriteDescriptorSet();
	write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write->dstBinding = binding;
	write->dstArrayElement = 0;
	write->descriptorCount = 1;
	return write;
}

VkDescriptorSetLayout agl::aglShader::GetDescriptorSetLayout()
{ return descriptorSetLayout; }

VkDescriptorPool agl::aglShader::GetDescriptorPool()
{ return descriptorPool; }

void agl::aglShader::AttachDescriptorSetLayout(VkDescriptorSetLayoutBinding binding, int b)
{
	if (bindings.size() <= b)
	{
		bindings.resize(b + 1);
	}
	bindings[b] = binding;
}

void agl::aglShader::AttachDescriptorPool(VkDescriptorPoolSize pool, int b)
{
	if (poolSizes.size() <= b)
	{
		poolSizes.resize(b+1);
	}
	poolSizes[b] = pool;
}

void agl::aglShader::AttachDescriptorWrite(VkWriteDescriptorSet* write, int frame, int binding)
{
	descriptorWrites[frame][binding] = (*write);
}

void agl::aglShader::AttachDescriptorWrites(std::vector<VkWriteDescriptorSet*> writes, int frame)
{
	int ctr = 0;
	for (auto write : writes)
	{
		AttachDescriptorWrite(write, frame, ctr);
		ctr++;
	}
}

void agl::aglShader::BindGraphicsPipeline(VkCommandBuffer commandBuffer)
{
	if (pushConstant) {
		vkCmdPushConstants(commandBuffer, GetPipelineLayout(), pushConstant->flags, 0, pushConstant->size, pushConstant->data);
	}
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	ppBuffer->Update(*postProcessing);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetPipelineLayout(), 0, 1, &descriptorSets[currentImage], 0, nullptr);
}

VkPipelineLayout agl::aglShader::GetPipelineLayout()
{
	return pipelineLayout;
}

void agl::aglShader::CreateGraphicsPipeline()
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	auto bindingDesc = aglVertex::GetBindingDescription();
	auto attributeDesc = aglVertex::GetAttributeDescriptions();

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
	rasterizer.cullMode = settings.cullFlags;
	rasterizer.frontFace = settings.frontFace;
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
	depthStencil.depthCompareOp = settings.depthCompare;
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

	if (pushConstant) {
		//setup push constants
		VkPushConstantRange push_constant;
		//this push constant range starts at the beginning
		push_constant.offset = 0;
		//this push constant range takes up the size of a MeshPushConstants struct
		push_constant.size = pushConstant->size;
		//this push constant range is accessible only in the vertex shader
		push_constant.stageFlags = pushConstant->flags;

		pipelineLayoutInfo.pPushConstantRanges = &push_constant;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
	}

	if (vkCreatePipelineLayout(GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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
	pipelineInfo.renderPass = settings.renderPass->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline.");
	}
}

void agl::aglShader::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<u32>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkResult result = vkCreateDescriptorSetLayout(GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
}

void agl::aglShader::CreateDescriptorPool()
{
	VkDescriptorPoolCreateInfo poolInfo{};

	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool.");
	}
}

void agl::aglShader::CreateDescriptorSet()
{
	for (auto port : ports)
	{
		if (port->type == DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			aglTexturePort* tport = static_cast<aglTexturePort*>(port);
			aglTexture* texture = tport->texture;
			if (texture) {
				for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
				{

					VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
					imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo->imageView = texture->textureImageView;
					imageInfo->sampler = texture->textureSampler;

					VkWriteDescriptorSet* descriptorWrite;

					descriptorWrite = CreateDescriptorSetWrite(i,port->binding);
					descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite->pImageInfo = imageInfo;

					AttachDescriptorWrite(descriptorWrite, i,port->binding);
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
	if (vkAllocateDescriptorSets(GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
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

		vkUpdateDescriptorSets(GetDevice(), static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
	}

}

void agl::aglShader::AttachTexture(aglTexture* texture, u32 binding)
{

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = binding;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkDescriptorPoolSize samplerPoolSize{};

	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

	AttachDescriptorPool(samplerPoolSize, binding);
	AttachDescriptorSetLayout(samplerLayoutBinding,binding);

	if (binding != -1)
	{

		aglTexturePort* port = static_cast<aglTexturePort*>(ports[binding]);

		port->texture = texture;
	}
}

agl::aglTexture::aglTexture(string path, VkFormat format)
{
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

	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	bool isCb = false;

	if (isCb)
	{
		for (uint32_t face = 0; face < 6; face++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	CreateVulkanImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, isCb);

	TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, true);
	CopyBufferToImage(stagingBuffer, textureImage, static_cast<u32>(texWidth), static_cast<u32>(texHeight), bufferCopyRegions.size(), bufferCopyRegions.data());
	TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, true);

	vkDestroy(vkDestroyBuffer, stagingBuffer);
	vkDestroy(vkFreeMemory, stagingBufferMemory);

	textureImageView = CreateImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT,isCb);
	CreateTextureSampler();
}

agl::aglTexture::aglTexture(aglShader* shader, aglTextureCreationInfo info)
{

	width = info.width;
	height = info.height;
	channels = info.channels;

	VkDeviceSize imageSize = info.width * info.height * info.channels;

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImageLayout layout;
	VkFormat format;

	if (info.isCubemap)
	{
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	else {
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		format = VK_FORMAT_R8G8B8A8_SRGB;
	}


	

	CreateVulkanImage(width, height, format, VK_IMAGE_TILING_OPTIMAL,  usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, info.isCubemap);

	textureImageView = CreateImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT, info.isCubemap);

	CreateTextureSampler();

	aglFramebuffer* fbo = new aglFramebuffer();
	if (info.isCubemap)
	{
		fbo = new aglFramebuffer(width, height, aglFramebufferCreationSettings{ format , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT });
	} else
	{
		fbo->imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	fbo->extent = VkExtent2D{ (u32) width,(u32)height};
	if (!info.isCubemap) {
		fbo->images.push_back(textureImage);
	}
	fbo->CreateFramebuffers();
	if (info.isCubemap)
	{
		TransitionImageLayout(fbo->images[0], fbo->imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,1, true);
	}

	// Pipeline layout
	struct PushBlock {
		glm::mat4 mvp;
		int drawId = 0;
	} pushBlock;

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[ ] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};


	if (info.isCubemap) {

		shader->pushConstant = new aglPushConstant();
		shader->pushConstant->flags = VK_SHADER_STAGE_VERTEX_BIT;
		shader->pushConstant->size = sizeof(pushBlock);
		shader->AttachTexture(info.baseCubemap, shader->GetBindingByName("equirectangularMap"));
	}


	shader->settings.renderPass = fbo->renderPass;

	shader->Setup();

	int layerCount = 1;

	if (info.isCubemap)
		layerCount = 6;

	TransitionImageLayout(textureImage, fbo->imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,layerCount, true);


	VkCommandBuffer cmd = baseSurface->commandBuffer->BeginSingleTimeCommands();

	fbo->renderPass->commandBuffer = new aglCommandBuffer;
	fbo->renderPass->commandBuffer->commandBuffers[0] = cmd;

	if (info.isCubemap) {

		

		for (int i = 0; i < 6; ++i)
		{
			pushBlock.mvp = captureProjection * captureViews[i];
			pushBlock.drawId = i;

			shader->pushConstant->data = &pushBlock;
			shader->pushConstant->size = sizeof(pushBlock);

			fbo->Bind(0,cmd);

			shader->BindGraphicsPipeline(cmd);

			((aglPrimitives*) agl_ext::installedExtensions[AGL_EXTENSION_PRIMITIVES_LAYER_NAME])->prims[aglPrimitives::CUBE]->Draw(cmd, 0);

			fbo->renderPass->End(cmd);

			TransitionImageLayout(fbo->images[0], fbo->imageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, false);

			CopyImageToImage(textureImage,fbo->images[0],i,1,width,height, false);

			TransitionImageLayout(fbo->images[0], fbo->imageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, false);
		}

	} else
	{
		fbo->Bind(0,cmd);

		shader->BindGraphicsPipeline(cmd);

		((aglPrimitives*) agl_ext::installedExtensions[AGL_EXTENSION_PRIMITIVES_LAYER_NAME])->prims[aglPrimitives::QUAD]->Draw(cmd, 0);

		fbo->renderPass->End(cmd);
	}

	TransitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount, false);

	baseSurface->commandBuffer->EndSingleTimeCommands(cmd);


	if (!info.isCubemap) {
		textureImage = fbo->images[0];
		textureImageView = fbo->imageViews[0];
		CreateTextureSampler();
	}
	else {

	}


}

VkImageView agl::aglTexture::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                                             bool isCubemap)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { aspectFlags, 0,1,0,1 };

	if (isCubemap)
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.subresourceRange.layerCount = 6;
	}

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image view");
	}

	return imageView;
}

void agl::aglTexture::CreateVulkanImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory,
	bool isCubemap)
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

	if (isCubemap)
	{
		imageInfo.arrayLayers = 6;
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void agl::aglTexture::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};

	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

agl::aglMesh::aglMesh(aiMesh* mesh)
{

	for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
	{
		aiVector3D v = mesh->mVertices[j];
		aiVector3D t = mesh->mTextureCoords[0][j];
		aiVector3D n = mesh->mNormals[j];

		aglVertex vtx = { {v.x, v.y,v.z}, {n.x,n.y,n.z}, {t.x, t.y} };

		vertices.push_back(vtx);
	}
	for (unsigned int j = 0; j < mesh->mNumFaces; j++)
	{
		aiFace face = mesh->mFaces[j];
		for (unsigned int l = 0; l < face.mNumIndices; l++)
			indices.push_back(face.mIndices[l]);
	}

	Setup();

	materialIndex = mesh->mMaterialIndex;

}

agl::aglMesh::aglMesh(aglMeshCreationData data)
{
	indices = data.indices;
	vertices = data.vertices;

	Setup();
}

void agl::aglMesh::Draw(VkCommandBuffer commandBuffer, u32 imageIndex)
{

	VkBuffer vertexBuffers[ ] = { vertexBuffer };
	VkDeviceSize offsets[ ] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<u32>(indices.size()), 1, 0, 0, 0);
}

void agl::aglMesh::Setup()
{

	{

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t) bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}
}

void agl::aglModel::Draw(aglCommandBuffer* commandBuffer, u32 imageIndex)
{
	for (aglMesh* mesh : meshes)
	{
		mesh->Draw(commandBuffer->GetCommandBuffer(imageIndex), imageIndex);
	}
}



void agl::agl_init(agl_details* details)
{

	agl::details = details;
	agl::postProcessing = new PostProcessingSettings;

	window = SDL_CreateWindow(details->applicationName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, details->Width, details->Height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);



	// Instance

	CreateInstance();

	// Debugging

	SetupDebugMessenger();

	// Surfacing

	CreateSurface();

	// Physical Device

	PickPhysicalDevice();

	// Logical Device

	CreateLogicalDevice();

	// Sync objects

	CreateSyncObjects();

  	baseSurface = new SurfaceDetails;


}

void agl::complete_init()
{


}


agl::aglModel::aglModel(string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	// If the import failed, report it
	if (scene == nullptr) {
		cout << (importer.GetErrorString());
		return;
	}

	for (int j = 0; j < scene->mNumTextures; ++j)
	{
		aiTexture* tex = scene->mTextures[j];
		cout << tex->mFilename.C_Str() << endl;

	}

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* aimesh = scene->mMeshes[i];

		aglMesh* mesh = new aglMesh(aimesh);

		meshes.push_back(mesh);
	}

	for (int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* aiMat = scene->mMaterials[i];

		vector<aglTextureRef> diffuseTextures = LoadMaterialTextures(aiMat, aiTextureType_DIFFUSE, path);
		vector<aglTextureRef> normalTextures = LoadMaterialTextures(aiMat, aiTextureType_NORMALS, path);

		aglMaterial* material = new aglMaterial;

		material->textures.insert({ aglMaterial::ALBEDO, diffuseTextures });
		material->textures.insert({ aglMaterial::NORMAL, normalTextures });

		materials.push_back(material);
	}
}

vector<agl::aglTextureRef> agl::aglModel::LoadMaterialTextures(aiMaterial* material, aiTextureType type, string modelPath)
{
	vector<aglTextureRef> textures;

	for (int i = 0; i < material->GetTextureCount(type); ++i)
	{
		aiString str;
		material->GetTexture(type, i, &str);

		string directory;
		string path;

		const size_t last_slash_idx = modelPath.rfind('\\');
		if (string::npos != last_slash_idx)
		{
			directory = modelPath.substr(0, last_slash_idx);
		}

		filesystem::path spath(str.C_Str());

		string spaths{ spath.filename().u8string() };

		path = directory + "//" + spaths;

		aglTextureRef ref = { path };

		textures.push_back(ref);

	}

	return textures;
}

void agl::UpdateFrame()
{
	static float lastTick = SDL_GetTicks64() / 1000.0f;
	float currentTime = SDL_GetTicks64() / 1000.0f;
	deltaTime = (currentTime - lastTick);

	if (deltaTime == 0.0f)
	{
		deltaTime = 1.0f / 60.0f;
	}

	lastTick = currentTime;

	DrawFrame();

	event = nullptr;
	frameCount++;
}

void agl::Destroy()
{
	vkDeviceWaitIdle(device);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	if (validationLayersEnabled)
	{
		DestroyDebugUtilsMessengerEXT(instance, DebugMessenger, nullptr);
	}


	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);


	SDL_DestroyWindow(window);

	SDL_Quit();
}