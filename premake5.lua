project "AuroraGraphicsLibrary"
   kind "SharedLib"
   language "C++"
   cppdialect "C++17"
   location "generated\\"
   compileas "C++"
   targetdir "%{_OPTIONS['corelocation']}/bin/%{cfg.buildcfg}"

   files { "include/**", "resources/shaders/**", ".editorconfig" }
   includedirs { "vendor/glfw/include/", "vendor/glm/", "include/", "vendor/assimp/include/", "vendor/","vendor/imgui/" }
   files {"vendor/imgui/backends/imgui_impl_glfw.*", "vendor/imgui/*"}
   files {"vendor/imgui/misc/debuggers/**", "vendor/imgui/misc/cpp/**"}
    libdirs {"vendor/glfw/lib-vc2022/","vendor/assimp/lib/Release/"}

    links {"glfw3.lib", "assimp-vc143-mt.lib"}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
      debugdir "./"
      runtime "Debug"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter {"options:gfxapi=vulkan"}
      defines {"GRAPHICS_VULKAN"}
      links {"vulkan-1.lib"}
      libdirs {"C:/VulkanSDK/1.3.250.0/Lib"}
      includedirs {"C:/VulkanSDK/1.3.250.0/Include"}
      removefiles {"include/GL/**"}
      files {"vendor/imgui/backends/imgui_impl_vulkan.*"}

   filter {"options:gfxapi=opengl"}
      defines {"GRAPHICS_OPENGL"}
      libdirs {"lib/glew"}
      links {"glew32.lib", "opengl32.lib"}
      removefiles {"include/spirv_reflect/**"}
      files {"vendor/imgui/backends/imgui_impl_opengl3.*"}
   
   