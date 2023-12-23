project "AuroraGraphicsLibrary"
   kind "SharedLib"
   language "C++"
   cppdialect "C++17"
   location "generated\\"
   compileas "C++"
   targetdir "%{_OPTIONS['corelocation']}/bin/%{cfg.buildcfg}"

   defines {"_CRT_SECURE_NO_WARNINGS"}

   files {
       "include/**",
        "resources/shaders/**",
         ".editorconfig",
         "%{_OPTIONS['utilslocation']}/include/**" ,
         "%{_OPTIONS['utilslocation']}/vendor/glm/glm/**" ,
         "%{_OPTIONS['utilslocation']}".."/vendor/sdl2/include/**", 
         "vendor/**.h"
   }
   includedirs { 
      "vendor/glfw/include/", 
       "include/",
        "vendor/assimp/include/",
         "vendor/","vendor/imgui/" ,
         "%{_OPTIONS['utilslocation']}".."/include", 
         "%{_OPTIONS['utilslocation']}".."/vendor/glm",
         "%{_OPTIONS['utilslocation']}".."/vendor/sdl2/include" 
      }
   
   files {"vendor/imgui/backends/imgui_impl_sdl2.*", "vendor/imgui/*"}
   files {"vendor/imgui/misc/debuggers/**", "vendor/imgui/misc/cpp/**"}
    libdirs {"vendor/glfw/lib-vc2022/","vendor/assimp/lib/Release/", "%{_OPTIONS['utilslocation']}".."/vendor/sdl2/lib" }

    links {"glfw3.lib", "assimp-vc143-mt.lib"}
    links {"vulkan-1.lib"}
    libdirs {"C:/VulkanSDK/1.3.250.0/Lib"}
    includedirs {"C:/VulkanSDK/1.3.250.0/Include"}
    files {"vendor/imgui/backends/imgui_impl_vulkan.*"}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
      debugdir "./"
      runtime "Debug"
      links {"SDL2d", "SDL2maind"}

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      links {"SDL2", "SDL2main"}


   