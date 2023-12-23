#if !defined(AGL_HPP)
#define AGL_HPP

#include <functional>
#include <map>
#include <optional>

#include "agl.hpp"
#include "agl.hpp"
#include "re.hpp"


enum aiTextureType : int;
struct aiMaterial;
struct aiMesh;

struct agl_details
{
	std::string applicationName;
	std::string engineName;
	u32 applicationVersion;
	u32 engineVersion;

	int Width, Height;
};

struct AURORA_API agl
{
	static void agl_init(agl_details* details);
	static void complete_init();
	inline static bool validationLayersEnabled = true;


	// Vulkan variables

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
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static bool CheckValidationLayerSupport();
	static std::vector<const char*> GetRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserData);

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	inline static const std::vector<const char*> validationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	inline static const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	inline static VkInstance instance = VK_NULL_HANDLE;
	inline static VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
	inline static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	inline static VkDevice device = VK_NULL_HANDLE;
	inline static VkQueue graphicsQueue = VK_NULL_HANDLE;
	inline static VkSurfaceKHR surface = VK_NULL_HANDLE;
	inline static VkQueue presentQueue = VK_NULL_HANDLE;
	inline static std::vector<VkSemaphore> imageAvailableSemaphores;
	inline static std::vector<VkSemaphore> renderFinishedSemaphores;
	inline static std::vector<VkFence> inFlightFences;
	inline static u32 currentImage;
	inline static bool closeWindow = false;
	IS float deltaTime = 1.0f / 60.0f;
	IS float frameCount = 1;


	static VkDevice GetDevice();

	struct aglShader;
	struct aglTexture;
	struct aglCommandBuffer;
	struct aglFramebuffer;
	struct aglSwapchain;

	struct AURORA_API SurfaceDetails
	{
		aglCommandBuffer* commandBuffer;
		aglFramebuffer* framebuffer;
		aglSwapchain* swapchain;
		u32 GetNextImageIndex();
		void Recreate();
		SurfaceDetails();
		void Destroy();
	private:
		void Create();
	};

private:
	IS SurfaceDetails* baseSurface;
	IS u32 currentFrame;
public:

	static SurfaceDetails* GetSurfaceDetails();
	static u32 GetCurrentImage();

	static void CreateInstance();
	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	                         VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int layerCount, bool endCmd);
	static void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height, u32 regionCount, VkBufferImageCopy* regions);
	static void CopyImageToImage(VkImage base, VkImage sub, int layer, int layerCount, int width, int height, bool endCmd);
	static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
	                                    VkFormatFeatureFlags features);

	static vec2 GetMainFramebufferSize();

	static VkFormat FindDepthFormat();
	static bool HasStencilComponent(VkFormat format);
	static void PresentFrame(u32 imageIndex);
	static void CreateSyncObjects();
	static void FramebufferResizeCallback(SDL_Window* window, int width, int height);
	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static void SetupDebugMessenger();
	static void PickPhysicalDevice();
	static bool IsDeviceSuitable(VkPhysicalDevice device);
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
	static void PollEvent(SDL_Event event);

	template <typename T>
	static void DestroyVulkanObject(std::function<void(VkDevice, T, const VkAllocationCallbacks*)> func, T object)
	{
		func(device, object, nullptr);
	}



	enum aglDescriptorType
	{
		DESCRIPTOR_TYPE_SAMPLER = 0,
		// = VK_DESCRIPTOR_TYPE_SAMPLER
		DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
		// = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
		// = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
		// = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
		// = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
		DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
		// = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
		DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
		// = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
		// = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
		// = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
		DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
		// = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
		DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
		// = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
		DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000 // = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
	};

	struct aglDescriptorPort
	{
		u32 binding;
		u32 set;

		std::string name;

		u32 count;

		aglDescriptorType type;

		aglTexture* texture = nullptr;
	};

	struct aglBufferSettings
	{
		VkShaderStageFlags flags;

		int bufferSize;

		std::string name;
		u32 binding;

		aglBufferSettings(VkShaderStageFlags flags, int bufferSize)
		{
			this->flags = flags;
			this->bufferSize = bufferSize;
			name = "";
			binding = 0;
		}

	};

	struct AURORA_API aglCommandBuffer
	{
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;
		VkCommandBuffer GetCommandBuffer(u32 currentImage) { return commandBuffers[currentImage]; }
		VkCommandBuffer currentBufferUsed=VK_NULL_HANDLE;


		aglCommandBuffer();

		void Begin(u32 currentImage);

		void End(u32 currentImage);

		void CreateMainBuffers() { CreateCommandBuffers(); }

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer);


		void CreateCommandPool();
		void CreateCommandBuffers();

		void Destroy();
	};

	struct aglMesh;
	struct aglRenderPass;


	struct aglRenderQueueEntry
	{
		agl::aglMesh* mesh;
		agl::aglShader* shader;
	};

	struct aglRenderQueue
	{
	private:
		std::vector<aglRenderQueueEntry> queueEntries;

		friend aglRenderPass;

		aglRenderPass* pass;

	public:

		aglRenderQueue(aglRenderPass* pass);

		void Push();

		void AttachQueueEntry(aglRenderQueueEntry entry)
		{
			queueEntries.push_back(entry);
		}
	};

	struct aglRenderPassSettings
	{
		VkImageLayout colorAttachmentLayout;
	};

	struct AURORA_API aglRenderPass
	{
		VkRenderPass renderPass;

		aglRenderPass(aglFramebuffer* framebuffer, aglRenderPassSettings settings);

		void AttachToCommandBuffer(aglCommandBuffer* buffer);

		void Begin(u32 imageIndex, VkCommandBuffer cmdBuf);

		void PushRenderQueue();
		void End(VkCommandBuffer cmdBuf);



		friend aglFramebuffer;

		void CreateRenderPass(aglRenderPassSettings settings);

		aglFramebuffer* framebuffer;
		aglCommandBuffer* commandBuffer;
		aglRenderQueue* renderQueue;
	};

	struct AURORA_API aglSwapchain
	{
		VkSwapchainKHR swapChain;
		aglFramebuffer* fbo;

		aglSwapchain();

		void CreateSwapChain();
		void Destroy();
		void Recreate();
	};

	struct aglFramebufferCreationSettings
	{
		VkFormat format;
		VkImageUsageFlags usage;
	};

	struct AURORA_API aglFramebuffer
	{
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
		VkFormat imageFormat;
		VkExtent2D extent;
		std::vector<VkFramebuffer> framebuffers;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		aglFramebuffer();

		aglFramebuffer(int width, int height, aglFramebufferCreationSettings settings);

		aglRenderPass* GetRenderPass() { return renderPass; };

		void Destroy();

		float GetAspect() { return extent.width / static_cast<float>(extent.height); }

		void Recreate();

		bool IsResized() { return Resized; }

		void CreateFramebuffers();

		void Bind(u32 imageIndex, VkCommandBuffer cmdBuf)
		{
			GetRenderPass()->Begin(imageIndex, cmdBuf);
		}

		bool Resized = false;


		void CreateImageViews();
		void CreateDepthResources();

		aglRenderPass* renderPass;
	};

	struct aglUniformBuffer;
	struct PostProcessingSettings;

	struct aglPushConstant
	{
		void* data;
		u32 size;
		VkShaderStageFlags flags;
	};

	struct AURORA_API aglShaderFactory
	{

		static void ReloadAllShaders();
		static void ReloadShader(u32 id);

		static void InsertShader(aglShader* shader);

	private:
		IS std::vector<aglShader*> loadedShaders;
	};

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

		aglShaderLevel(std::string code, aglShaderType type, aglShader* parent);

		VkPipelineShaderStageCreateInfo stageInfo;

		void Destroy();


		friend aglShader;
		aglShader* parent;
	};

	struct aglShaderSettings
	{
		struct SettingPaths
		{
			std::string vertexPath;
			std::string fragmentPath;
			std::string geometryPath;
			std::string computePath;
		} paths;

		VkCullModeFlags cullFlags = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkCompareOp depthCompare = VK_COMPARE_OP_LESS;
		aglRenderPass* renderPass = baseSurface->framebuffer->renderPass;
	};

	struct AURORA_API aglShader
	{

		u32 id;

		aglShaderLevel* vertModule = nullptr;
		aglShaderLevel* fragModule = nullptr;
		aglShaderLevel* compModule=nullptr;

		std::string vertexCode, fragmentCode, computeCode;

		friend aglShaderLevel;

		void Setup();
		virtual void Create();

		aglShaderSettings settings;
		aglUniformBuffer* ppBuffer = nullptr;
		aglPushConstant* pushConstant=nullptr;

		aglShader(aglShaderSettings settings);

		static std::string CompileGLSLToSpirV(std::string path);

	public:
		u32 GetBindingByName(std::string n);

		virtual void Destroy();

		void Recreate();

		std::vector<VkDescriptorPoolSize> poolSizes;

		static VkWriteDescriptorSet* CreateDescriptorSetWrite(int frame, int binding);

		void BindGraphicsPipeline(VkCommandBuffer commandBuffer);
		VkPipelineLayout GetPipelineLayout();
		VkDescriptorSetLayout GetDescriptorSetLayout();
		VkDescriptorPool GetDescriptorPool();
		void AttachDescriptorSetLayout(VkDescriptorSetLayoutBinding binding, int bindingIdx);
		void AttachDescriptorPool(VkDescriptorPoolSize pool, int binding);

		void AttachDescriptorWrite(VkWriteDescriptorSet* write, int frame, int binding);

		void AttachDescriptorWrites(std::vector<VkWriteDescriptorSet*> writes, int frame);

		void CreateGraphicsPipeline();
		void CreateComputePipeline();
		void CreateDescriptorSetLayout();
		void CreateDescriptorPool();
		void CreateDescriptorSet();
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<std::vector<VkWriteDescriptorSet>> descriptorWrites;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipeline mainPipeline;
		VkDescriptorPool descriptorPool;
		std::vector<aglDescriptorPort*> ports;



		bool setup=false;

		virtual void AttachTexture(aglTexture* texture, u32 binding = -1);
	};

	struct AURORA_API aglComputeShader : aglShader
	{

		aglComputeShader(aglShaderSettings settings);

		static void BeginDispatchField();
		void Dispatch(u32 imageIndex, vec3 groupCount);
		static void EndDispatchField();

		void Create() override;
		void Destroy() override;
		void AttachTexture(aglTexture* texture, u32 binding) override;

		IS VkQueue computeQueue;
		IS std::vector<VkCommandBuffer> computeCommandBuffers;
		IS std::vector<VkSemaphore> computeFinishedSemaphores;
		IS std::vector<VkFence> computeInFlightFences;
		IS bool AreComputeShadersUsed=false;
		IS bool ObjectsCreated = false;

		void CreateCommandBuffers();
		void CreateSyncObjects();

		static std::vector<VkSemaphore> submittedSemaphores;
	};

	struct AURORA_API aglStorageBuffer
	{

		aglStorageBuffer(aglBufferSettings settings);

		void Destroy();

		void Update(void* data, size_t dataSize);
		void* GetData();

		void AttachToShader(aglShader* shader, u32 bindingIdx);

		unsigned ubo;



		void CreateBinding(VkDescriptorSetLayoutBinding bind, int bindingIdx);

		void CreatePoolSize(VkDescriptorPoolSize poolSz, int bindingIdx);

		VkBuffer GetBuffer(int frame);

		VkDescriptorSetLayout setLayout;
		std::vector<VkBuffer> buffers;
		std::vector<VkDeviceMemory> bufferMemory;
		std::vector<void*> mappedBuffers;

		VkDescriptorSetLayoutBinding binding;
		VkDescriptorPoolSize poolSize;



		aglShader* shader;

		aglBufferSettings settings={0,0};

	};

	// Combined variables

	inline static SDL_Window* window = nullptr;
	inline static agl_details* details = nullptr;
	inline static SDL_Event* event=nullptr;

	struct aglVertex
	{
		vec3 position;
		vec3 normal;
		vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(aglVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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




	};

	struct aglTextureRef
	{
		std::string path;
	};

	struct aglTextureCreationInfo
	{
		int width, height, channels;
		bool isCubemap;
		aglTexture* baseCubemap;
	};

	struct AURORA_API aglTexture
	{
		aglTexture(std::string path, VkFormat format);
		aglTexture(aglShader* shader, aglTextureCreationInfo info);
		aglTexture(aglTextureCreationInfo info);

		aglTexture(aglTextureRef ref) : aglTexture(ref.path, VK_FORMAT_R8G8B8A8_SRGB)
		{
		}

		static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
		                                   bool IsCubemap=false);
		static void CreateVulkanImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
		                              VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
		                              VkDeviceMemory& imageMemory, bool IsCubemap=false);
		void CreateTextureSampler();

		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;


		u32 id;



		int width, height, channels;

		std::string path;


		friend aglShader;
	};

	struct aglMaterial
	{
		enum TextureType
		{
			ALBEDO = 0,
			NORMAL
		};

		std::map<TextureType, std::vector<aglTextureRef>> textures;

		aglShader* shader=nullptr;
	};

	struct aglMeshCreationData
	{
		std::vector<aglVertex> vertices;
		std::vector<unsigned> indices;
	};

	struct aglMesh
	{
		std::vector<aglVertex> vertices;
		std::vector<unsigned> indices;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;


		u32 materialIndex;

		aglMesh(aiMesh* mesh);
		aglMesh(aglMeshCreationData data);

		void Draw(VkCommandBuffer commandBuffer, u32 imageIndex);

		static void ExportAsCode(aglMesh* mesh, std::string path);


		std::vector<aglTexture> textures;

		friend class aglModel;

	private:
		void Setup();
	};

	struct aglModel
	{
		std::vector<aglMesh*> meshes;
		std::vector<aglMaterial*> materials;

		aglModel(std::string path);

		void Draw(aglCommandBuffer* commandBuffer, u32 imageIndex);


		std::vector<aglTextureRef> LoadMaterialTextures(aiMaterial* material, aiTextureType type, std::string path);
	};

	struct AURORA_API aglUniformBuffer
	{
		aglUniformBuffer(aglShader* shader, aglBufferSettings settings);

		void Destroy();

		void Update(void* data, size_t dataSize);

		void AttachToShader(aglShader* shader, u32 bindingIdx);

		unsigned ubo;



		void CreateBinding(VkDescriptorSetLayoutBinding bind, int bindingIdx);

		void CreatePoolSize(VkDescriptorPoolSize poolSz, int bindingIdx);

		VkBuffer GetUniformBuffer(int frame);

		VkDescriptorSetLayout setLayout;
		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> ubMemory;
		std::vector<void*> mappedUbs;

		VkDescriptorSetLayoutBinding binding;
		VkDescriptorPoolSize poolSize;



		aglShader* shader;

		aglBufferSettings settings = {0,0};
	};

	inline static struct PostProcessingSettings
	{
		alignas(4) float gammaCorrection = 2.2f;
		alignas(4) float radiancePower = 1;
	}* postProcessing = nullptr;

	static void UpdateFrame();
	static void Destroy();
};


// AGL_HPP

#endif
