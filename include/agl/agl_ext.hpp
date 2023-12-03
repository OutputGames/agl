#if !defined(AGL_EXTENSIONS_HPP)
#define AGL_EXTENSIONS_HPP

#include <functional>

#include "re.hpp"

using namespace std;


#define AGL_EXTENSION_IMGUI_LAYER_NAME "agl_ext_imgui"

struct agl_ext
{
	struct aglExtension
	{
		virtual void Install();
		virtual void Refresh();
		virtual void LateRefresh();
		virtual void Uninstall();
		virtual ~aglExtension() = default;
		string name = "agl_ext_undefined";
	};

	inline static unordered_map<string, aglExtension*> installedExtensions = unordered_map<string,aglExtension*>(0);

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

struct aglImGuiExtension : agl_ext::aglExtension
{
	void Install() override;
	void Refresh() override;
	void Uninstall() override;
	void LateRefresh() override;
	string name = AGL_EXTENSION_IMGUI_LAYER_NAME;

	static void DragVec3(string text,vec3& v);
	static void ColorEditVec3(string text, vec3& v);
	static void Dockspace();

private:
	VkDescriptorPool imguiPool;
};


#endif // AGL_EXTENSIONS_HPP
