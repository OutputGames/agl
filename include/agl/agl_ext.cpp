#include "agl_ext.hpp"

#include "agl.hpp"
#include "ext.hpp"
#include "maths.hpp"

#include "aurora/utils/fs.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "agl/models/sphere.hpp"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"

using namespace std;


void agl_ext::aglExtension::Install()
{
	cout << "Installing extension: " << name << endl;
}

void agl_ext::aglExtension::Refresh()
{
}

void agl_ext::aglExtension::LateRefresh()
{
}

void agl_ext::aglExtension::Uninstall()
{
	cout << "Uninstalling extension: " << name << endl;
}

void aglImGuiExtension::Install()
{
	aglExtension::Install();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	VkDescriptorPoolSize pool_sizes[ ] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	vkCreateDescriptorPool(agl::device, &pool_info, nullptr, &imguiPool);


	ImGui_ImplSDL2_InitForVulkan(agl::window);
	ImGui_ImplVulkan_InitInfo init_info{};

	init_info.Instance = agl::instance;
	init_info.PhysicalDevice = agl::physicalDevice;
	init_info.Device = agl::device;
	init_info.Queue = agl::graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, agl::GetSurfaceDetails()->framebuffer->renderPass->renderPass);

	ImGui_ImplVulkan_CreateFontsTexture();

}
void aglImGuiExtension::Refresh()
{
	aglExtension::Refresh();

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
}

void aglImGuiExtension::Uninstall()
{
	aglExtension::Uninstall();

	vkDestroyDescriptorPool(agl::device, imguiPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
}

void aglImGuiExtension::LateRefresh()
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), agl::GetSurfaceDetails()->commandBuffer->commandBuffers[agl::GetCurrentImage()]);

}

void aglImGuiExtension::DragVec3(string text,vec3& v)
{
	float p[3] = { v.x, v.y, v.z };

	ImGui::DragFloat3(text.c_str(), p,0.1);

	v = { p[0],p[1],p[2] };
}

void aglImGuiExtension::ColorEditVec3(string text, vec3& v)
{
	float p[3] = { v.x, v.y, v.z };

	ImGui::ColorEdit3(text.c_str(), p);

	v = { p[0],p[1],p[2] };
}

void aglImGuiExtension::Dockspace()
{
	bool p_open = true;
	// Variables to configure the Dockspace example.
	static bool opt_fullscreen = true; // Is the Dockspace full-screen?
	static bool opt_padding = false; // Is there padding (a blank space) between the window edge and the Dockspace?
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None; // Config flags for the Dockspace

	// In this example, we're embedding the Dockspace into an invisible parent window to make it more configurable.
	// We set ImGuiWindowFlags_NoDocking to make sure the parent isn't dockable into because this is handled by the Dockspace.
	//
	// ImGuiWindowFlags_MenuBar is to show a menu bar with config options. This isn't necessary to the functionality of a
	// Dockspace, but it is here to provide a way to change the configuration flags interactively.
	// You can remove the MenuBar flag if you don't want it in your app, but also remember to remove the code which actually
	// renders the menu bar, found at the end of this function.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	// Is the example in Fullscreen mode?
	if (opt_fullscreen)
	{
		// If so, get the main viewport:
		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Set the parent window's position, size, and viewport to match that of the main viewport. This is so the parent window
		// completely covers the main viewport, giving it a "full-screen" feel.
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		// Set the parent window's styles to match that of the main viewport:
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); // No corner rounding on the window
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // No border around the window

		// Manipulate the window flags to make it inaccessible to the user (no titlebar, resize/move, or navigation)
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	else
	{
		// The example is not in Fullscreen mode (the parent window can be dragged around and resized), disable the
		// ImGuiDockNodeFlags_PassthruCentralNode flag.
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so the parent window should not have its own background:
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// If the padding option is disabled, set the parent window's padding size to 0 to effectively hide said padding.
	if (!opt_padding)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::Begin("DockSpace", &p_open, window_flags);

	// Remove the padding configuration - we pushed it, now we pop it:
	if (!opt_padding)
		ImGui::PopStyleVar();

	// Pop the two style rules set in Fullscreen mode - the corner rounding and the border size.
	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// Check if Docking is enabled:
	ImGuiIO& io = ImGui::GetIO();

	// If it is, draw the Dockspace with the DockSpace() function.
	// The GetID() function is to give a unique identifier to the Dockspace - here, it's "MyDockSpace".
	ImGuiID dockspace_id = ImGui::GetID("EngineDS");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

}

ImGuiContext* aglImGuiExtension::GetContext()
{
	return ImGui::GetCurrentContext();
}

void aglImGuiExtension::ProcessSDLEvent(SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

void aglPrimitives::Install()
{

	vector<aglPrimitiveType> primitives = {
		aglPrimitiveType::CUBE,
		aglPrimitiveType::SPHERE,
		aglPrimitiveType::QUAD,
		aglPrimitiveType::CAPSULE,
	};

	/*
	{

		agl::aglMesh* sphereMesh = agl::aglModel("resources/models/sphere.obj").meshes[0];

		agl::aglMesh::ExportAsCode(sphereMesh, "include/agl/models/sphere.hpp");
	}
	*/


	for (auto primitive : primitives)
	{
		aglPrimitiveType type = primitive;

		switch (type)
		{
		case CUBE:
			prims[type] = GenerateCube(1, 1, 1);
			break;
		case QUAD:
			prims[type] = GenerateQuad();
			break;
		case SPHERE:
			prims[type] = GenerateSphere(1,3,3);
			break;
		case CAPSULE:
			prims[type] = GenerateCapsule(2, 1);
			break;
		}

		prims[type]->path = GetPrimNames()[type];
	}
}

void aglPrimitives::DrawPrimitive(aglPrimitiveType type, VkCommandBuffer cmdBuf)
{
	agl::aglMesh* mesh = prims[type];

	mesh->Draw(cmdBuf, agl::GetCurrentImage());
}

std::map<aglPrimitives::aglPrimitiveType, agl::aglMesh*> aglPrimitives::GetPrims()
{
	return prims;
}

std::vector<std::string> aglPrimitives::GetPrimNames()
{
	vector<string> primNames = {
	"PRIM_CUBE",
	"PRIM_SPHERE",
	"PRIM_QUAD",
		"PRIM_CAPSULE"
	};

	return primNames;
}

agl::aglMesh* aglPrimitives::GenerateCube(float width, float height, float length)
{
	float vertices[ ] = {
	-width / 2, -height / 2, length / 2,
	width / 2, -height / 2, length / 2,
	width / 2, height / 2, length / 2,
	-width / 2, height / 2, length / 2,
	-width / 2, -height / 2, -length / 2,
	-width / 2, height / 2, -length / 2,
	width / 2, height / 2, -length / 2,
	width / 2, -height / 2, -length / 2,
	-width / 2, height / 2, -length / 2,
	-width / 2, height / 2, length / 2,
	width / 2, height / 2, length / 2,
	width / 2, height / 2, -length / 2,
	-width / 2, -height / 2, -length / 2,
	width / 2, -height / 2, -length / 2,
	width / 2, -height / 2, length / 2,
	-width / 2, -height / 2, length / 2,
	width / 2, -height / 2, -length / 2,
	width / 2, height / 2, -length / 2,
	width / 2, height / 2, length / 2,
	width / 2, -height / 2, length / 2,
	-width / 2, -height / 2, -length / 2,
	-width / 2, -height / 2, length / 2,
	-width / 2, height / 2, length / 2,
	-width / 2, height / 2, -length / 2
	};

	float texcoords[ ] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	float normals[ ] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};



	vector<agl::aglVertex> verts;
	vector<unsigned> indices;

	for (int i = 0; i < std::size(vertices); i+=3)
	{
		agl::aglVertex vtx = {};

		float v[3] = { vertices[i], vertices[i + 1], vertices[i + 2] };
		float n[3] = { normals[i], normals[i + 1], normals[i + 2] };

		vtx.position = { v[0],v[1],v[2] };
		vtx.normal = { n[0],n[1],n[2] };

		verts.push_back(vtx);

	}

	int j = 0;
	for (int i = 0; i < std::size(texcoords); i+=2)
	{

		verts[j].texCoord = { texcoords[i], texcoords[i + 1] };

		j++;
	}

	int k = 0;

	int idxCt = ((size(vertices)/3) / 2) * 3;

	indices.resize(idxCt);

	// Indices can be initialized right now
	for (int i = 0; i < idxCt; i += 6)
	{
		indices[i] = 4 * k;
		indices[i + 1] = 4 * k + 1;
		indices[i + 2] = 4 * k + 2;
		indices[i + 3] = 4 * k;
		indices[i + 4] = 4 * k + 2;
		indices[i + 5] = 4 * k + 3;

		k++;
	}


	agl::aglMesh* mesh = new agl::aglMesh({verts,indices});

	return mesh;
}

agl::aglMesh* aglPrimitives::GenerateSphere(float radius, int rings, int slices)
{
	/*
	if (rings >= 3 && slices >= 3)
	{
		par_shapes_mesh* sphere = par_shapes_create_parametric_sphere(slices, rings);
		par_shapes_scale(sphere, radius, radius, radius);

		vector<agl::aglVertex> verts;
		vector<unsigned> indices;

		for (int i = 0; i < sphere->npoints; i+=3)
		{
			agl::aglVertex vtx = {};




			vtx.position = { sphere->points[i], sphere->points[i+1], sphere->points[i+2]};
			vtx.normal = { sphere->normals[i], sphere->normals[i + 1], sphere->normals[i] + 2 };

			verts.push_back(vtx);

		}

		int j = 0;
		for (int i = 0; i < sphere->ntriangles*2; i += 2)
		{

			//verts[j].texCoord = { sphere->tcoords[i], sphere->tcoords[i + 1] };

			j++;
		}



		par_shapes_free_mesh(sphere);

		agl::aglMesh* mesh = new agl::aglMesh({ verts,indices });

		return mesh;
	}
	return nullptr;
	*/

	vector<agl::aglVertex> vertices(SPHERE_VERTEX_COUNT);
	vector<unsigned> indices(SPHERE_TRIANGLE_COUNT * 3);
	for (int i = 0; i < SPHERE_VERTEX_COUNT; ++i)
	{

		float p[3];
		for (int j = 0; j < 3; ++j)
		{
			p[j] = SPHERE_VERTEX_DATA[(i * 3) + j];
		}

		float n[3];
		for (int j = 0; j < 3; ++j)
		{
			n[j] = SPHERE_NORMAL_DATA[(i * 3) + j];
		}

		float t[2];
		for (int j = 0; j < 2; ++j)
		{
			t[j] = SPHERE_TEXCOORD_DATA[(i * 2) + j];
		}

		vertices[i].position = make_vec3(p);
		vertices[i].normal = make_vec3(n);
		vertices[i].texCoord = make_vec2(t);
	}
	for (int i = 0; i < SPHERE_TRIANGLE_COUNT * 3; ++i)
	{
		indices[i] = SPHERE_INDEX_DATA[i];
	}

	return new agl::aglMesh(agl::aglMeshCreationData{ vertices, indices });
}

agl::aglMesh* aglPrimitives::GenerateQuad()
{
	vector<agl::aglVertex> verts = {
		agl::aglVertex{{-1.0f, -1.0f,1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		agl::aglVertex{{1.0f, -1.0f,1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		agl::aglVertex{{1.0f, 1.0f,1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		agl::aglVertex{{-1.0f, 1.0f,1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}

	};
	vector<unsigned> indices = {
		0,1,2,2,3,0
	};

	return new agl::aglMesh({ verts,indices });
}

agl::aglMesh* aglPrimitives::GenerateCapsule(float height, float radius)
{
	par_shapes_mesh* mesh = par_shapes_create_cylinder(32, 16);
	par_shapes_scale(mesh, radius, radius, height);
	//par_shapes__subdivide(mesh

	{
		par_shapes_mesh* sphere = par_shapes_create_parametric_sphere(32, 8);
		par_shapes_scale(sphere, radius, radius, radius);
		par_shapes_translate(sphere, 0, 0, height);

		par_shapes_merge_and_free(mesh, sphere);
	}

	{
		par_shapes_mesh* sphere = par_shapes_create_parametric_sphere(32, 8);
		par_shapes_scale(sphere, radius, radius, radius);

		par_shapes_merge_and_free(mesh, sphere);
	}

	par_shapes_compute_normals(mesh);
	par_shapes_translate(mesh, 0, 0, (-height / 2)-(radius));

	u32 num_vertices = mesh->npoints * 3;
	u32 num_indices = mesh->ntriangles * 3;

	vector<agl::aglVertex> vertices;
	vector<u32> indices;

	vertices.resize(num_vertices);
	indices.resize(num_indices);

	for (int i = 0; i < num_indices; ++i)
	{
		indices[i] = mesh->triangles[i];
	}

	for (int i = 0; i < num_vertices*3; i += 3)
	{
		agl::aglVertex v{};

		float pos[3] = { mesh->points[i], mesh->points[i + 1], mesh->points[i + 2] };
		v.position = make_vec3(pos);

		float norm[3] = { mesh->normals[i], mesh->normals[i + 1], mesh->normals[i + 2] };
		v.normal = make_vec3(norm);

		vertices[i/3] = v;
	}

	for (int i = 0; i < num_vertices * 2; i += 2)
	{
		agl::aglVertex v = vertices[i / 2];

		float tex[2] = { mesh->tcoords[i], mesh->tcoords[i + 1]};
		v.texCoord = make_vec2(tex);

		vertices[i / 2] = v;
	}


	agl::aglMeshCreationData data = {vertices, indices};

	agl::aglMesh* aglMesh = new agl::aglMesh(data);

	return aglMesh;
}
