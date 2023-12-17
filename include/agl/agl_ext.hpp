#if !defined(AGL_EXTENSIONS_HPP)
#define AGL_EXTENSIONS_HPP

#include <functional>

#include "agl.hpp"
#include "re.hpp"


#define AGL_EXTENSION_IMGUI_LAYER_NAME "agl_ext_imgui"
#define AGL_EXTENSION_PRIMITIVES_LAYER_NAME "agl_ext_prims"

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
	static void InstallExtension(T* extension)
	{

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

private:
	VkDescriptorPool imguiPool;
};

struct AURORA_API aglPrimitives : agl_ext::aglExtension
{
	enum aglPrimitiveType
	{
		CUBE = 0,
		SPHERE,
		QUAD
	};

	std::string name = AGL_EXTENSION_PRIMITIVES_LAYER_NAME;

	inline static std::map<aglPrimitiveType,agl::aglMesh*> prims;

	void Install() override;

	void DrawPrimitive(aglPrimitiveType type, VkCommandBuffer cmdBuf);

private:

	static agl::aglMesh* GenerateCube(float width,  float height, float length);
	static agl::aglMesh* GenerateSphere(float radius, int rings, int slices);
	static agl::aglMesh* GenerateQuad();

};


#endif // AGL_EXTENSIONS_HPP
