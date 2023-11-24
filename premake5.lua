-- premake5.lua


newoption {
   trigger = "gfxapi",
   value = "API",
   description = "Choose a particular 3D API for rendering",
   allowed = {
      { "opengl",    "OpenGL" },
      { "vulkan",  "Vulkan" },
   },
   default = "opengl"
}

workspace "CantDoVulkan"
   configurations { "Debug", "Release" }
   platforms { "x32", "x64" }

   filter { "platforms:x32" }
       system "Windows"
       architecture "x86"
   
   filter { "platforms:x64" }
       system "Windows"
       architecture "x64"

project "CantDoVulkan"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   location "generated\\"
   compileas "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "src/**", "include/**", "resources/shaders/**", ".editorconfig" }
   includedirs { "vendor/glfw/include/", "vendor/glm/", "include/", "vendor/assimp/include/", "vendor/" }

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

   filter {"options:gfxapi=opengl"}
      defines {"GRAPHICS_OPENGL"}
      libdirs {"lib/glew"}
      links {"glew32.lib", "opengl32.lib"}
      removefiles {"include/spirv_reflect/**"}
   
   