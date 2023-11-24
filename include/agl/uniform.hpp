#ifndef UNIFORM_HPP
#define UNIFORM_HPP

struct UniformBufferSettings
{
	VkShaderStageFlags flags;
};

template <typename T>
struct UniformBuffer
{
	static UniformBuffer* CreateNewBuffer(Shader* shader, UniformBufferSettings settings);

	void Destroy();

	void Update(T data, u32 currentImage);

	void CreateBinding(VkDescriptorSetLayoutBinding bind);
	void CreatePoolSize(VkDescriptorPoolSize poolSz);
	VkBuffer GetUniformBuffer(int frame) { return uniformBuffers[frame]; }

	void AttachToShader(Shader* shader);
	void DscrptrSets(Shader* shader);

private:

	VkDescriptorSetLayout setLayout;
	vector<VkBuffer> uniformBuffers;
	vector<VkDeviceMemory> ubMemory;
	vector<void*> mappedUbs;

	Shader* shader;
	VkDescriptorSetLayoutBinding binding;
	VkDescriptorPoolSize poolSize;

	UniformBufferSettings settings;


};

template <typename T>
UniformBuffer<T>* UniformBuffer<T>::CreateNewBuffer(Application* application, Shader* shader, UniformBufferSettings settings)
{
	UniformBuffer<T>* uni = new UniformBuffer<T>;

	uni->application = application;
	uni->shader = shader;
	uni->settings = settings;

	// UB creation

	VkDeviceSize bufferSize = sizeof(T);

	uni->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uni->ubMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uni->mappedUbs.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		application->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uni->uniformBuffers[i], uni->ubMemory[i]);

		vkMapMemory(application->GetDevice(), uni->ubMemory[i], 0, bufferSize, 0, &uni->mappedUbs[i]);
	}

	// Descriptor Pool

	//uni->CreateDescriptorPool();

	// Descriptor Sets

	//uni->CreateDescriptorSets();

	return uni;
}

template <typename T>
void UniformBuffer<T>::Destroy()
{



	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(application->GetDevice(), uniformBuffers[i], nullptr);
		vkFreeMemory(application->GetDevice(), ubMemory[i], nullptr);
	}
}

template <typename T>
void UniformBuffer<T>::Update(T data, u32 currentImage)
{
	memcpy(mappedUbs[currentImage], &data, sizeof(data));
}

template <typename T>
void UniformBuffer<T>::CreateBinding(VkDescriptorSetLayoutBinding bind)
{
	binding = bind;
	shader->AttachDescriptorSetLayout(binding);
}

template <typename T>
void UniformBuffer<T>::CreatePoolSize(VkDescriptorPoolSize poolSz)
{
	poolSize = poolSz;
	shader->AttachDescriptorPool(poolSize);
}

template <typename T>
void UniformBuffer<T>::AttachToShader(Shader* shader)
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
		bufferInfo->range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet* descriptorWrite;


		descriptorWrite = shader->CreateDescriptorSetWrite(i);
		descriptorWrite->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite->pBufferInfo = bufferInfo;

		shader->AttachDescriptorWrite(descriptorWrite, i);
	}
}

template <typename T>
void UniformBuffer<T>::DscrptrSets(Shader* shader)
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = GetUniformBuffer(i);
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(T);

		VkWriteDescriptorSet descriptorWrite;


		descriptorWrite = shader->CreateDescriptorSetWrite(i);
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;

		shader->AttachDescriptorWrite(descriptorWrite, i);
	}
}

#endif
