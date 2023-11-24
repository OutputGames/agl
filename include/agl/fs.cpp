#include "fs.hpp"

std::string ReadString(const std::string path)
{
    const std::ifstream input_stream(path, std::ios_base::binary);

    if (input_stream.fail()) {
        throw std::runtime_error("Failed to open file located at " + path);
    }

    std::stringstream buffer;
    buffer << input_stream.rdbuf();

    return buffer.str();
}