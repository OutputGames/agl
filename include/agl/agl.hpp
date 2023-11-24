#if !defined(AGL_HPP)
#define AGL_HPP

#include <functional>

#include "re.hpp"

using namespace std;

enum aiTextureType : int;
struct aiMaterial;
struct aiMesh;

struct agl_details
{
	string applicationName;
	string engineName;
	u32 applicationVersion;
	u32 engineVersion;

	int Width, Height;
};

struct agl {
    static void agl_init(agl_details* details);
	static void complete_init();
#ifdef GRAPHICS_VULKAN
    inline static bool validationLayersEnabled = true;
#endif

	// Vulkan variables

#ifdef GRAPHICS_VULKAN
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

	static bool CheckValidationLayerSupport();
	static vector<cc*> GetRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	inline static const vector<cc*> validationLayers{
	"VK_LAYER_KHRONOS_validation"
	};

	inline static const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	inline static VkInstance instance = VK_NULL_HANDLE;
    inline static VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
    inline static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    inline static VkDevice device= VK_NULL_HANDLE;
    inline static VkQueue graphicsQueue= VK_NULL_HANDLE;
    inline static VkSurfaceKHR surface= VK_NULL_HANDLE;
    inline static VkQueue presentQueue= VK_NULL_HANDLE;
	inline static vector<VkSemaphore> imageAvailableSemaphores;
	inline static vector<VkSemaphore> renderFinishedSemaphores;
	inline static vector<VkFence> inFlightFences;
	inline static u32 currentFrame = 0;
	inline static u32 currentImage;


	static VkDevice GetDevice()
	{
		return device;
	}

	struct aglShader;
	struct aglTexture;
	struct aglCommandBuffer;
	struct aglFramebuffer;

	struct SurfaceDetails
	{

		aglCommandBuffer* commandBuffer;
		aglFramebuffer* framebuffer;
		u32 GetNextImageIndex();
	};

	inline static SurfaceDetails* baseSurface = nullptr;

	static void CreateInstance();
	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	static void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);
	static VkFormat FindSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	static VkFormat FindDepthFormat();
	static bool HasStencilComponent(VkFormat format);
	static void PresentFrame(u32 imageIndex);
	static void CreateSyncObjects();
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static void SetupDebugMessenger();
	static void PickPhysicalDevice();
	static bool IsDeviceSuitable(const VkPhysicalDevice device);
	static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	static void CreateLogicalDevice();
	static void CreateSurface();
	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	static void record_command_buffer(u32 imageIndex);
	static void FinishRecordingCommandBuffer(u32 imageIndex);
	static void DrawFrame();

	template<typename T>
	static inline void DestroyVulkanObject(function<void(VkDevice, T, const VkAllocationCallbacks*)> func, T object)
	{
		func(device, object, nullptr);
	}

	enum aglShaderType
	{
		VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
		FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
		GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
		COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
	};


	struct aglShaderLevel
	{
		VkShaderModule module;

		aglShaderLevel(string code, aglShaderType type, aglShader* parent);

		VkPipelineShaderStageCreateInfo stageInfo;

		void Destroy();

	
		friend aglShader;
		aglShader* parent;
	};

	enum aglDescriptorType
	{
		DESCRIPTOR_TYPE_SAMPLER = 0,        // = VK_DESCRIPTOR_TYPE_SAMPLER
		DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,        // = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,        // = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,        // = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,        // = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
		DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,        // = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
		DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,        // = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,        // = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,        // = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
		DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,        // = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
		DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,        // = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
		DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000 // = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
	};

	struct aglDescriptorPort
	{
		u32 binding;
		u32 set;

		string name;

		u32 count;

		aglDescriptorType type;
	};

	struct aglTexturePort : aglDescriptorPort
	{
		aglTexture* texture;
	};

	struct aglCommandBuffer
	{
		VkCommandPool commandPool;
		vector<VkCommandBuffer> commandBuffers;
		VkCommandBuffer GetCommandBuffer(u32 currentImage) { return commandBuffers[currentImage]; }


		aglCommandBuffer();

		void Begin(u32 currentImage);

		void End(u32 currentImage);

		void CreateMainBuffers() { CreateCommandBuffers(); }

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer);

	

		void CreateCommandPool();
		void CreateCommandBuffers();
	};

	struct aglRenderPass
	{
		VkRenderPass renderPass;

		aglRenderPass(aglFramebuffer* framebuffer);

		void AttachToCommandBuffer(aglCommandBuffer* buffer);

		void Begin(u32 imageIndex);

	

		friend aglFramebuffer;

		void CreateRenderPass();

		aglFramebuffer* framebuffer;
		aglCommandBuffer* commandBuffer;
	};

	struct aglFramebuffer
	{
		VkSwapchainKHR swapChain;
		vector<VkImage> swapChainImages;
		vector<VkImageView> swapChainImageViews;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		vector<VkFramebuffer> swapChainFramebuffers;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		aglFramebuffer();

		aglRenderPass* GetRenderPass() { return renderPass; };

		void Destroy();

		float GetAspect() { return swapChainExtent.width / (float)swapChainExtent.height; }

		void Recreate();

		bool IsResized() { return Resized; }

		void CreateFramebuffers();

		void Bind(u32 imageIndex)
		{
			GetRenderPass()->Begin(imageIndex);
		}

		bool Resized = false;

	

		void CreateSwapChain();
		void CreateImageViews();
		void CreateDepthResources();

		aglRenderPass* renderPass;
	};



	struct aglShader
	{
		aglShaderLevel* vertModule;
		aglShaderLevel* fragModule;

		std::string vertexCode, fragmentCode;

		friend aglShaderLevel;

		void Setup();

		struct aglShaderSettings
		{
			string vertexPath;
			string fragmentPath;

			VkCullModeFlags cullFlags = VK_CULL_MODE_BACK_BIT;
			VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			VkCompareOp depthCompare = VK_COMPARE_OP_LESS;
		};

		aglShaderSettings* settings;

		aglShader(aglShaderSettings* settings);

	public:

		u32 GetBindingByName(string n);

		void Destroy();


		vector<VkDescriptorPoolSize> poolSizes;
		VkWriteDescriptorSet* CreateDescriptorSetWrite(int frame) { VkWriteDescriptorSet* write = new VkWriteDescriptorSet(); write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; write->dstBinding = descriptorWrites[frame].size(); write->dstArrayElement = 0; write->descriptorCount = 1; return write; }
		void BindGraphicsPipeline(VkCommandBuffer commandBuffer);
		void BindDescriptor(VkCommandBuffer commandBuffer, u32 currentImage);
		VkPipelineLayout GetPipelineLayout();
		VkDescriptorSetLayout GetDescriptorSetLayout() { return descriptorSetLayout; }
		VkDescriptorPool GetDescriptorPool() { return descriptorPool; }
		void AttachDescriptorSetLayout(VkDescriptorSetLayoutBinding binding) { bindings.push_back(binding); }
		void AttachDescriptorPool(VkDescriptorPoolSize pool) { poolSizes.push_back(pool); }
		void AttachDescriptorWrite(VkWriteDescriptorSet* write, int frame) { descriptorWrites[frame].push_back(*write); }
		void AttachDescriptorWrites(vector<VkWriteDescriptorSet*> writes, int frame)
		{
			for (auto write : writes)
			{
				descriptorWrites[frame].push_back(*write);
			}
		}
		void CreateGraphicsPipeline();
		void CreateDescriptorSetLayout();
		void CreateDescriptorPool();
		void CreateDescriptorSet();
		vector<VkDescriptorSetLayoutBinding> bindings;
		vector<VkDescriptorSet> descriptorSets;
		vector<vector<VkWriteDescriptorSet>> descriptorWrites;
		vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipeline graphicsPipeline;
		VkDescriptorPool descriptorPool;
		vector<aglDescriptorPort* > ports;

		void AttachTexture(aglTexture* texture, u32 binding = -1);
	};

	struct aglUniformBufferSettings
	{
		VkShaderStageFlags flags;
	};

	template <typename T>
	struct aglUniformBuffer
	{
		aglUniformBuffer(aglShader* shader, aglUniformBufferSettings settings)
		{
			this->shader = shader;
			this->settings = settings;

			// UB creation

			VkDeviceSize bufferSize = sizeof(T);

			uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
			ubMemory.resize(MAX_FRAMES_IN_FLIGHT);
			mappedUbs.resize(MAX_FRAMES_IN_FLIGHT);

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], ubMemory[i]);

				vkMapMemory(GetDevice(), ubMemory[i], 0, bufferSize, 0, &mappedUbs[i]);
			}
		}

		void Destroy()
		{
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroyBuffer(GetDevice(), uniformBuffers[i], nullptr);
				vkFreeMemory(GetDevice(), ubMemory[i], nullptr);
			}
		}

		void Update(T data)
		{
			memcpy(mappedUbs[currentFrame], &data, sizeof(data));
		}

		void CreateBinding(VkDescriptorSetLayoutBinding bind)
		{
			binding = bind;
			shader->AttachDescriptorSetLayout(binding);
		}

		void CreatePoolSize(VkDescriptorPoolSize poolSz)
		{
			poolSize = poolSz;
			shader->AttachDescriptorPool(poolSize);
		}

		VkBuffer GetUniformBuffer(int frame) { return uniformBuffers[frame]; }

		void AttachToShader(aglShader* shader)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding{};
			uboLayoutBinding.binding = shader->poolSizes.size();
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = settings.flags;
			uboLayoutBinding.pImmutableSamplers = nullptr;

			VkDescriptorPoolSize uboPoolSize{};

			uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboPoolSize.descriptorCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

			shader->AttachDescriptorPool(uboPoolSize);

			CreateBinding(uboLayoutBinding);

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{

				VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
				bufferInfo->buffer = GetUniformBuffer(i);
				bufferInfo->offset = 0;
				bufferInfo->range = sizeof(T);

				VkWriteDescriptorSet* descriptorWrite;


				descriptorWrite = shader->CreateDescriptorSetWrite(i);
				descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrite->pBufferInfo = bufferInfo;

				shader->AttachDescriptorWrite(descriptorWrite, i);
			}
		}

		VkDescriptorSetLayout setLayout;
		vector<VkBuffer> uniformBuffers;
		vector<VkDeviceMemory> ubMemory;
		vector<void*> mappedUbs;

		aglShader* shader;
		VkDescriptorSetLayoutBinding binding;
		VkDescriptorPoolSize poolSize;

		aglUniformBufferSettings settings;


	};

	
#endif

	// OpenGL variables

#ifdef GRAPHICS_OPENGL

	struct aglShader
	{
		u32 id;

		struct aglShaderSettings
		{
			string vertexPath;
			string fragmentPath;

		};

		aglShaderSettings* settings;

		aglShader(aglShaderSettings* settings);
	};

#endif

	// Combined variables

	inline static GLFWwindow* window = nullptr;
	inline static agl_details* details = nullptr;

	struct aglVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;

#ifdef GRAPHICS_VULKAN
		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(aglVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(aglVertex, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(aglVertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(aglVertex, texCoord);

			return attributeDescriptions;
		}
#endif

#ifdef GRAPHICS_OPENGL

#endif

	};

	struct aglTextureRef
	{
		string path;
	};

	struct aglTexture
	{
		aglTexture(string path);
		aglTexture(aglTextureRef ref) : aglTexture(ref.path) {};

#ifdef GRAPHICS_VULKAN
		static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device);
		static void CreateVulkanImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice device, VkPhysicalDevice physicalDevice);
		void CreateTextureSampler();

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
#endif

#ifdef GRAPHICS_OPENGL
		u32 id;
#endif

		int width, height, channels;

		string path;

	

		friend aglShader;

	};

	struct aglMaterial
	{
		enum TextureType
		{
			ALBEDO = 0,
			NORMAL
		};

		map<TextureType, vector<aglTextureRef>> textures;
	};

	struct aglMesh
	{
		vector<aglVertex> vertices;
		vector<u32> indices;

#ifdef GRAPHICS_VULKAN
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
#endif

		aglShader* shader;

		u32 materialIndex;

		aglMesh(aiMesh* mesh);

#ifdef GRAPHICS_VULKAN
		void Draw(aglCommandBuffer* commandBuffer, u32 imageIndex);
#endif

	

		vector<aglTexture> textures;

		friend class aglModel;

	};

	struct aglModel
	{

		vector<aglMesh*> meshes;
		vector<aglMaterial*> materials;

		aglModel(string path);

#ifdef GRAPHICS_VULKAN
		void Draw(aglCommandBuffer* commandBuffer, u32 imageIndex);
#endif

		void SetShader(aglShader* shader);

	

		vector<aglTextureRef> LoadMaterialTextures(aiMaterial* material, aiTextureType type, string path);
	};

	static void UpdateFrame();
	static void Destroy();

};


#endif // AGL_HPP
