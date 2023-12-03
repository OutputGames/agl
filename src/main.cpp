
#include <filesystem>

#include "imgui.h"
#include "agl/agl.hpp"
#include "agl/agl_ext.hpp"
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



	agl::aglShader::aglShaderSettings* settings = new agl::aglShader::aglShaderSettings{ "resources/shaders/glsltest/test-vert.spv", "resources/shaders/glsltest/test-frag.spv" };

	agl::aglModel* model = new agl::aglModel("resources\\models\\Player00\\Player00.fbx");

	Camera* camera = new Camera;

	vector<agl::aglUniformBuffer<UniformBufferObject>*> uniformBuffers;
	vector<agl::aglUniformBuffer<LightingSettings>*> lightingSettings;

	for (agl::aglMesh* mesh : model->meshes)
	{
		agl::aglMaterial* material = model->materials[mesh->materialIndex];

		agl::aglShader* shader = new agl::aglShader(settings);


		agl::aglTexture* texture = new agl::aglTexture(material->textures[agl::aglMaterial::ALBEDO][0]);

		agl::aglTexture* nrmTexture = new agl::aglTexture(material->textures[agl::aglMaterial::NORMAL][0]);

		agl::aglUniformBufferSettings settings = {};



#if GRAPHICS_VULKAN

		settings.flags = VK_SHADER_STAGE_VERTEX_BIT;

		agl::aglUniformBuffer<UniformBufferObject>* uniformBuffer = new agl::aglUniformBuffer<UniformBufferObject>(shader, settings);
		uniformBuffers.push_back(uniformBuffer);

		agl::aglUniformBuffer<LightingSettings>* lightingSetting = new agl::aglUniformBuffer<LightingSettings>(shader, { VK_SHADER_STAGE_FRAGMENT_BIT });
		lightingSettings.push_back(lightingSetting);

		lightingSetting->AttachToShader(shader, shader->GetBindingByName("lightingSettings"));

  		uniformBuffer->AttachToShader(shader,shader->GetBindingByName("ubo"));

		shader->AttachTexture(texture, shader->GetBindingByName("albedo"));

		shader->AttachTexture(nrmTexture, shader->GetBindingByName("normalMap"));

		shader->Setup();

		shader->CreateDescriptorSet();

#else

		settings.binding = 0;
		settings.name = "UniformBufferObject";

		agl::aglUniformBuffer<UniformBufferObject>* uniformBuffer = new agl::aglUniformBuffer<UniformBufferObject>(shader, settings);
		uniformBuffers.push_back(uniformBuffer);

		agl::aglUniformBuffer<LightingSettings>* lightingSetting = new agl::aglUniformBuffer<LightingSettings>(shader, { "LightingSettings",shader->GetBindingByName("LightingSettings.lightingSettings")});
		lightingSettings.push_back(lightingSetting);

		shader->AttachTexture(texture, shader->GetBindingByName("albedo"));

		shader->AttachTexture(nrmTexture, shader->GetBindingByName("normalMap"));

#endif


		mesh->shader = shader;

	}

	LightingSettings lightingSetting{};

	lightingSetting.lights[0] = (Light{{2,2,2}, {1,1,1}});
	lightingSetting.lights[1] = (Light{{-2,-2,-2}, {1,1,1}});
	lightingSetting.lightAmount = 1;
	lightingSetting.albedo = { 1,1,1 };
	lightingSetting.metallic = 0.0;
	lightingSetting.roughness = 0.5;

	agl::complete_init();

	agl_ext::InstallExtension<aglImGuiExtension>(new aglImGuiExtension());

    try {

	    while (!glfwWindowShouldClose(agl::window))
	    {

			agl_ext::installedExtensions[AGL_EXTENSION_IMGUI_LAYER_NAME]->Refresh();

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
			int x, y;
			glfwGetWindowSize(agl::window, &x, &y);

			extents = { x,y };

#endif


			camera->Update(extents);

			UniformBufferObject ubo{};
			ubo.model = mat4(1.0);

			//ubo.model = rotate(ubo.model, radians(90.0f), {0,1,0});

			ubo.view = camera->GetViewMatrix();
			ubo.proj = camera->GetProjectionMatrix();
			ubo.normalMatrix = transpose(inverse(glm::mat3(ubo.model)));
			ubo.camPos = camera->position;

#ifdef GRAPHICS_VULKAN
			ubo.proj[1][1] *= -1;
#endif

			for (auto uniformBuffer : uniformBuffers) {

				uniformBuffer->Update(ubo);

			}

			for (auto lighting_setting : lightingSettings)
			{
				lighting_setting->Update(lightingSetting);
			}

#ifdef GRAPHICS_VULKAN
  
			agl::record_command_buffer(agl::currentFrame);

			model->Draw(agl::baseSurface->commandBuffer, agl::currentFrame);

			//aglImGuiExtension::Dockspace();

			szt lightSize = alignof(LightingSettings);

			ImGui::Begin("Lighting Settings");

			if (ImGui::CollapsingHeader("Lights")) {

				int ctr = 0;

				for (auto&& light : lightingSetting.lights)
				{
					string name = "Light " + to_string(ctr);

					ImGui::BeginChild(name.c_str());

					ImGui::Text(name.c_str());

					aglImGuiExtension::DragVec3("Position", light.position);
					aglImGuiExtension::ColorEditVec3("Color", light.color);

					ImGui::DragFloat("Power", &light.power);

					ImGui::EndChild();
					ctr++;
				}
			}

			if (ImGui::CollapsingHeader("Material settings"))
			{
				ImGui::DragFloat("Roughness", &lightingSetting.roughness, 0.1f, 0, 1);
				ImGui::DragFloat("Metallic", &lightingSetting.metallic, 0.1f, 0, 1);
			}

			if (ImGui::CollapsingHeader("Post processing settings"))
			{
				ImGui::DragFloat("Gamma correction", &agl::postProcessing->gammaCorrection, 0.1);
				ImGui::DragFloat("Radiance Power", &agl::postProcessing->radiancePower, 0.1);
			}


			ImGui::End();

			//ImGui::End();

			agl_ext::installedExtensions[AGL_EXTENSION_IMGUI_LAYER_NAME]->LateRefresh();

			agl::FinishRecordingCommandBuffer(agl::currentFrame);
#endif

#ifdef GRAPHICS_OPENGL

			agl::postProcessing->gammaCorrection = 1;
			agl::postProcessing->radiancePower = 10;


			model->Draw();
#endif
				
			agl::UpdateFrame();
	    }

		agl_ext::UninstallAll();
		agl::Destroy();


    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}