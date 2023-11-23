#ifndef APP_VK
#define APP_VK

#include <functional>

#include "gfx.hpp"
#include "re.hpp"



using namespace std;

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger);

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator);

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;
};

struct ApplicationDetails
{
	string applicationName;
	string engineName;
	u32 applicationVersion;
	u32 engineVersion;

	int Width, Height;
};

class Application
{
public:

	Application(ApplicationDetails* details);

	void InitWindow(uint32_t WIDTH, uint32_t HEIGHT);

	void InitVulkan();

	void MainLoop();

	void Clean();

	void CreateInstance();

	bool CheckValidationLayerSupport();

	static vector<cc*> GetRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserData);

	static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void SetupDebugMessenger();

	void PickPhysicalDevice();

	bool IsDeviceSuitable(const VkPhysicalDevice device);

	static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

	void CreateLogicalDevice();

	void CreateSurface();

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	void RecordCommandBuffer(u32 imageIndex);

	void DrawFrame();

	void PresentFrame(u32 imageIndex);

	u32 GetNextImageIndex();

	void CreateSyncObjects();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

	void UpdateUniformBuffer(u32 currentImage);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer);

	GLFWwindow* window = nullptr;
	VkInstance instance;
	VkDebugUtilsMessengerEXT DebugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkSurfaceKHR surface;
	VkQueue presentQueue;

	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;
	vector<VkFence> inFlightFences;
	u32 currentFrame = 0;

	Model* model;

	Camera* camera;

	CommandBuffer* commandBuffer;

	Framebuffer* framebuffer;

	ApplicationDetails* applicationDetails;

	VkDevice GetDevice();


	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);

	template<typename T>
	void DestroyVulkanObject(std::function<void(VkDevice, T, const VkAllocationCallbacks*)> func, T object);


	VkFormat FindSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkFormat FindDepthFormat();

	bool HasStencilComponent(VkFormat format);

	void Run()
	{
		MainLoop();
		Clean();
	}
};

template <typename T>
void Application::DestroyVulkanObject(std::function<void(VkDevice, T, const VkAllocationCallbacks*)> func, T object)
{
	func(device, object, nullptr);
}


#endif
