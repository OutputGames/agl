
#ifndef GFX_VK
#define GFX_VK

#include "re.hpp"

#include "fs.hpp"
//#include "uniform.hpp"

enum aiTextureType : int;
struct aiMaterial;
struct Shader;
struct Texture;
struct aiMesh;
struct Framebuffer;
class Application;
using namespace std;
using namespace glm;


enum ShaderType
{
	VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
	FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
	GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT,
	COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
};

struct ShaderLevel
{
	VkShaderModule module;

	ShaderLevel(string code, Application* app, ShaderType type, Shader* parent);

	VkPipelineShaderStageCreateInfo stageInfo;

	void Destroy();

private:
	friend class Shader;
	Application* application;
	Shader* parent;
};

enum DescriptorType
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

struct DescriptorPort
{
	u32 binding;
	u32 set;

	string name;

	u32 count;

	DescriptorType type;
};

struct TexturePort : DescriptorPort
{
	Texture* texture;
};

struct CommandBuffer
{
	VkCommandPool commandPool;
	vector<VkCommandBuffer> commandBuffers;
	VkCommandBuffer GetCommandBuffer(u32 currentImage) { return commandBuffers[currentImage]; }


	CommandBuffer(Application* application);

	void Begin(u32 currentImage);

	void End(u32 currentImage);

	void CreateMainBuffers() { CreateCommandBuffers(); }

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer vkCommandBuffer);

private:

	void CreateCommandPool();
	void CreateCommandBuffers();

	Application* application;
};

struct RenderPass
{
	VkRenderPass renderPass;

	RenderPass(Application* app, Framebuffer* framebuffer);

	void AttachToCommandBuffer(CommandBuffer* buffer);

	void Begin(u32 imageIndex);

private:

	friend Framebuffer;

	void CreateRenderPass();

	Application* application;
	Framebuffer* framebuffer;
	CommandBuffer* commandBuffer;
};

struct Framebuffer
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

	Framebuffer(Application* app);

	RenderPass* GetRenderPass() { return renderPass; };

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

private:
	
	void CreateSwapChain();
	void CreateImageViews();
	void CreateDepthResources();


	Application* application;
	RenderPass* renderPass;
};

struct Shader
{
private:

	ShaderLevel* vertModule;
	ShaderLevel* fragModule;

	std::string vertexCode, fragmentCode;

	Application* application;

	friend Application;
	friend ShaderLevel;

	void Setup();

	struct ShaderSettings
	{
		string vertexPath;
		string fragmentPath;

		VkCullModeFlags cullFlags=VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkCompareOp depthCompare=VK_COMPARE_OP_LESS;
	};

	ShaderSettings* settings;

	Shader(ShaderSettings* settings, Application* appl);

public:

	u32 GetBindingByName(string n);

	void Destroy();


	vector<VkDescriptorPoolSize> poolSizes;
	VkWriteDescriptorSet* CreateDescriptorSetWrite(int frame) { VkWriteDescriptorSet* write = new VkWriteDescriptorSet(); write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; write->dstBinding = descriptorWrites[frame].size(); write->dstArrayElement = 0; write->descriptorCount = 1; return write;}
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
	vector<DescriptorPort* > ports;

	void AttachTexture(Texture* texture, u32 binding = -1);

};


struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
		array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

};

struct UniformBufferObject
{
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
	vec3 camPos;
};

struct Camera
{
	vec3 position={0,0,2};
	vec3 target;
	vec3 up = { 0.0,1.0,0.0 };
	vec3 right = { 1.0,0,0 };
	float fov = 45.0f;

	void Update(VkExtent2D extents);

	mat4 GetViewMatrix()
	{
		return view;
	}

	mat4 GetProjectionMatrix()
	{
		return proj;
	}

private:
	mat4 view=mat4(1.0), proj=mat4(1.0);
};

struct TextureRef
{
	string path;
};

struct Texture
{
	Texture( Application* app, string path);
	Texture(Application* app, TextureRef ref) : Texture(app, ref.path) {};

	static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device);
	static void CreateVulkanImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice device, VkPhysicalDevice physicalDevice);

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	int width, height, channels;

	string path;

private:

	friend Application;
	friend Shader;

	void CreateTextureSampler();




	Application* application;
};

struct Material
{
	enum TextureType
	{
		ALBEDO = 0,
		NORMAL
	};

	map<TextureType, vector<TextureRef>> textures;
};

struct Mesh
{
	vector<Vertex> vertices;
	vector<u32> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	Shader* shader;

	u32 materialIndex;

	Mesh(aiMesh* mesh, Application* app);

	void Draw(CommandBuffer* commandBuffer, u32 imageIndex);

private:

	vector<Texture> textures;

	friend class Model;

	Application* application;
};

struct Model
{

	vector<Mesh*> meshes;
	vector<Material*> materials;

	Model(string path, Application* app);

	void Draw(CommandBuffer* commandBuffer, u32 imageIndex);

	void SetShader(Shader* shader);

private:

	vector<TextureRef> LoadMaterialTextures(aiMaterial* material, aiTextureType type, string path);

	Application* application;
};

#endif
