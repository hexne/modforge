/********************************************************************************
* @Author : hexne
* @Date   : 2026/01/16 15:09:25
********************************************************************************/

export module modforge.directory;
import std;

NAMESPACE_BEGIN
export class Directory {
    std::filesystem::path cur_root_;
    int deep_;
    bool is_relative_path_;

    auto get_deep(const std::filesystem::path& path) const {
        auto str = path.lexically_relative(cur_root_).lexically_normal().generic_string();
        return std::ranges::count(str.begin(), str.end(), '/') + 1;
    }

    static auto parse_extension_filter(const std::string& extension) {
        std::unordered_set<std::string> ret;
        if (extension.empty())
            return ret;
        for (auto &&part : std::views::split(extension, ';')) {
            ret.emplace(part.begin(), part.end());
        }
        return ret;
    }
public:
    explicit Directory(const std::string& path, bool is_relative_path = false, int deep = 0) {
        deep_ = deep;
        is_relative_path_ = is_relative_path;
        cur_root_ = std::filesystem::absolute(path);
    }
    Directory(int deep) : Directory(".", false, deep) {  }
    Directory(bool is_relative_path) : Directory(".", is_relative_path, 0) {  }

    Directory() : Directory(".") {  }

    void traverser(std::function<void(std::filesystem::path)> &&callback) const {
        for (const auto& cur_path : std::filesystem::recursive_directory_iterator(cur_root_)) {
            callback(cur_path);
        }
    }

    // only_file 无完整路径
    // only_name 无类型扩展
    auto files(const std::string &extension = "", bool only_file = false, bool only_name = false) const {
        std::vector<std::string> ret;
        auto extension_filter = parse_extension_filter(extension);
        bool has_extension_filter = !extension_filter.empty();


        traverser([&](auto &&path) {
            std::filesystem::path file = path;
            if (std::filesystem::is_directory(path))
                return;
            if (deep_ != 0 and get_deep(path) > deep_)
                return;

            if (has_extension_filter) {
                auto ext = path.extension().string();

                if (!ext.empty()) {
                    ext = ext.substr(1);
                }
                
                if (extension_filter.find(ext) == extension_filter.end())
                    return;
            }
            if (is_relative_path_)
                file = file.lexically_relative(cur_root_);
            if (only_file)
                file = path.filename();
            if (only_name)
                file = path.stem();
            ret.emplace_back(file.string());
        });
        return ret;
    }
    auto directorys(bool only_name = false) const {
        std::vector<std::string> ret;
        traverser([&](auto &&path) {
            if (!std::filesystem::is_directory(path))
                return;
            int deep = get_deep(path);
            if (deep_ != 0 and deep > deep_)
                return;
            auto dir = path;
            if (is_relative_path_)
                dir = dir.lexically_relative(cur_root_);
            if (only_name)
                dir = path.stem();
            ret.emplace_back(dir.string());
        });
        return ret;
    }

};
NAMESPACE_END
