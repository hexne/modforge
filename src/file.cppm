/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/13 13:33:09
********************************************************************************/

module;
export module file;
import std;
import utils;

NAMESPACE_BEGIN

export struct File {

    static std::string read_file(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw format_runtime_error("Could not open file {}", path);

        auto length = file.tellg();
        file.seekg(std::ios::beg);
        std::string ret(length, 0);
        file.read(&ret[0], length);
        return ret;
    }

    static void write_file(const std::filesystem::path &path, const std::string &content) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            throw std::runtime_error(std::format("Could not open file {}", path));

        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file)
            throw std::runtime_error(std::format("Failed to write file {}", path));
    }

};
NAMESPACE_END