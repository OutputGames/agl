
#include <filesystem>

#include "agl/agl.hpp"
#include "agl/ext.hpp"
#include "agl/maths.hpp"
#include "agl/re.hpp"


int main(void) {

    system("C:/Users/chris/Downloads/AuroraGraphicsLibrary/shadercompile.bat");
        

    cout << std::filesystem::current_path() << endl;

    agl_details* details = new agl_details;

    details->applicationName = "agl testing";
    details->engineName = "AuroraGraphicsEngine";
#ifdef GRAPHICS_VULKAN
    details->engineVersion = VK_MAKE_VERSION(1, 0, 0);
    details->applicationVersion = VK_MAKE_VERSION(1, 0, 0);
#endif
    details->Width = 800;
    details->Height = 600;

	agl::agl_init(details);

#ifdef GRAPHICS_VULKAN

	agl::aglShader::aglShaderSettings* settings = new agl::aglShader::aglShaderSettings{ "resources/shaders/glsltest/test-vert.spv", "resources/shaders/glsltest/test-frag.spv" };

	agl::aglModel* model = new agl::aglModel("resources\\models\\Player00\\Player00.fbx");

	Camera* camera = new Camera;

	vector<agl::aglUniformBuffer<UniformBufferObject>*> uniformBuffers;

	for (agl::aglMesh* mesh : model->meshes)
	{
		agl::aglMaterial* material = model->materials[mesh->materialIndex];

		agl::aglShader* shader = new agl::aglShader(settings);


		agl::aglTexture* texture = new agl::aglTexture(material->textures[agl::aglMaterial::ALBEDO][0]);

		agl::aglTexture* nrmTexture = new agl::aglTexture(material->textures[agl::aglMaterial::NORMAL][0]);

		agl::aglUniformBuffer<UniformBufferObject>* uniformBuffer = new agl::aglUniformBuffer<UniformBufferObject>(shader, { VK_SHADER_STAGE_VERTEX_BIT });
		uniformBuffers.push_back(uniformBuffer);
		uniformBuffer->AttachToShader(shader);

		shader->AttachTexture(texture, shader->GetBindingByName("albedo"));

		shader->AttachTexture(nrmTexture, shader->GetBindingByName("normalMap"));

		shader->Setup();

		shader->CreateDescriptorSet();

		mesh->shader = shader;

	}

	agl::complete_init();

#endif

    try {

	    while (!glfwWindowShouldClose(agl::window))
	    {
#ifdef GRAPHICS_VULKAN
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			float radius = 2.5f;

			float cx = sin(time) * radius;
			float cz = cos(time) * radius;

			camera->position = { cx,2.0,cz };
			camera->target = { 0,1,0 };

			vec2 extents;

#ifdef GRAPHICS_VULKAN
			extents = aglMath::ConvertExtents(agl::baseSurface->framebuffer->swapChainExtent);

#else
			
#endif


			camera->Update(extents);

			UniformBufferObject ubo{};
			ubo.model = mat4(1.0);

			//ubo.model = rotate(ubo.model, radians(90.0f), {0,1,0});

			ubo.view = camera->GetViewMatrix();
			ubo.proj = camera->GetProjectionMatrix();
			ubo.proj[1][1] *= -1;

			for (auto uniformBuffer : uniformBuffers) {

				uniformBuffer->Update(ubo);

			}

#ifdef GRAPHICS_VULKAN
  
			agl::record_command_buffer(agl::currentFrame);

			model->Draw(agl::baseSurface->commandBuffer, agl::currentFrame);

			agl::FinishRecordingCommandBuffer(agl::currentFrame);
#endif

#endif

			agl::UpdateFrame();
	    }

		agl::Destroy();


    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}