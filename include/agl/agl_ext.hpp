#if !defined(AGL_EXTENSIONS_HPP)
#define AGL_EXTENSIONS_HPP

#include <functional>

#include "agl.hpp"
#include "re.hpp"


#define AGL_EXTENSION_IMGUI_LAYER_NAME "agl_ext_imgui"
#define AGL_EXTENSION_PRIMITIVES_LAYER_NAME "agl_ext_prims"

struct ImGuiContext;
struct TransformationBuffer;
AURORA_API struct agl_ext
{
	struct AURORA_API aglExtension
	{
		virtual void Install();
		virtual void Refresh();
		virtual void LateRefresh();
		virtual void Uninstall();
		virtual ~aglExtension() = default;
		std::string name = "agl_ext_undefined";
	};

	inline static std::unordered_map<std::string, aglExtension*> installedExtensions = std::unordered_map<std::string,aglExtension*>(0);

	static void Refresh()
	{
		for (auto&& installed_extension : installedExtensions)
			installed_extension.second->Refresh();
	}

	static void LateRefresh()
	{
		for (auto&& installed_extension : installedExtensions)
			installed_extension.second->LateRefresh();
	}

	static void UninstallAll()
	{
		for (auto&& installed_extension : installedExtensions)
			installed_extension.second->Uninstall();
	}

	template <typename T>
	static void InstallExtension()
	{
		T* extension = new T();
		installedExtensions.insert({ extension->name, extension });
		extension->Install();
	}
};

struct AURORA_API aglImGuiExtension : agl_ext::aglExtension
{
	void Install() override;
	void Refresh() override;
	void Uninstall() override;
	void LateRefresh() override;
	std::string name = AGL_EXTENSION_IMGUI_LAYER_NAME;

	static void DragVec3(std::string text,vec3& v);
	static void ColorEditVec3(std::string text, vec3& v);
	static void Dockspace();

	static ImGuiContext* GetContext();
	static void ProcessSDLEvent(SDL_Event* event);

private:
	VkDescriptorPool imguiPool;
};

struct AURORA_API aglPrimitives : agl_ext::aglExtension
{
	enum aglPrimitiveType
	{
		CUBE = 0,
		SPHERE,
		QUAD,
		CAPSULE
	};

	std::string name = AGL_EXTENSION_PRIMITIVES_LAYER_NAME;

	void Install() override;

	void DrawPrimitive(aglPrimitiveType type, VkCommandBuffer cmdBuf);

	static std::map<aglPrimitiveType, agl::aglMesh*> GetPrims();

	static std::vector<std::string> GetPrimNames();

private:

	inline static std::map<aglPrimitiveType, agl::aglMesh*> prims;

	static agl::aglMesh* GenerateCube(float width,  float height, float length);
	static agl::aglMesh* GenerateSphere(float radius, int rings, int slices);
	static agl::aglMesh* GenerateQuad();
	static agl::aglMesh* GenerateCapsule(float height, float radius);

};


#endif // AGL_EXTENSIONS_HPP
