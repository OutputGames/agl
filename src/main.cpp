
#include <filesystem>

#include "renderengine/agl.hpp"
#include "renderengine/re.hpp"

int main(void) {

    system("C:/Users/chris/Downloads/ICantDoVulkan/shadercompille.bat");
        

    cout << std::filesystem::current_path() << endl;

    agl_details* details = new agl_details;

    details->applicationName = "NorthernLightsTesting";
    details->engineName = "NorthenLights";
    details->engineVersion = VK_MAKE_VERSION(1, 0, 0);
    details->applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    details->Width = 800;
    details->Height = 600;

	agl::agl_init(details);

    try {




    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}