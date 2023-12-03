
cd C:/Users/chris/Downloads/AuroraGraphicsLibrary/

C:/VulkanSDK/1.3.250.0/Bin/spirv-cross --version 330 --no-es resources/shaders/glsltest/test-vert.spv --output resources/shaders/glsltest/DECOMPILED_test.vert
C:/VulkanSDK/1.3.250.0/Bin/spirv-cross --version 330 --no-es resources/shaders/glsltest/test-frag.spv --output resources/shaders/glsltest/DECOMPILED_test.frag

echo "Decompiled shaders successfully."