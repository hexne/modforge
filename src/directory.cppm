/********************************************************************************
* @Author : hexne
* @Date   : 2026/01/16 15:09:25
********************************************************************************/

module;
export module modforge.directory;
import std;
import modforge.string_utils;

template <typename P>
concept has_generic_display_string = requires(const P& p) { p.generic_display_string(); };

template <typename P>
std::string path_string(const P& p) {
    if constexpr (has_generic_display_string<P>) {
        return p.generic_display_string();
    } else {
        return p.generic_string();
    }
}

NAMESPACE_BEGIN
export class Directory {
    std::filesystem::path root_{};
    int deep_{};

    auto get_deep(const std::filesystem::path& path) const {
        auto str = path_string(path.lexically_relative(root_).lexically_normal());
        return std::ranges::count(str.begin(), str.end(), '/') + 1;
    }

public:

    Directory(const std::filesystem::path &root, int deep = -1) : root_(root), deep_(deep) {  }

    // 查找所有后缀文件
    // extent表示后缀集合
    // only name 表示没有路径和后缀
    // only file 表示没有文件夹
    std::vector<std::string> files(std::string extent, bool only_name = false, bool only_file = false) {
        auto extents = StringUtils::split(extent, ';');

        std::vector<std::string> ret;
        auto check_extent = [&](std::string ext) -> bool {
            return std::ranges::find(extents, ext) != extents.end();
        };

        traverser_directory([&](std::filesystem::path path) {
            // 如果要有文件夹就会自动处理，不要的时候外层已经过滤过了
            auto ext = path_string(path.extension());
            if (!check_extent(ext))
                return;

            if (only_name)
                ret.push_back(path.filename());
            else if (!only_name)
                ret.push_back(path.lexically_normal());
        }, only_file);
        return ret;
    }

    void traverser_directory(const std::function<void(std::filesystem::path)> callback, bool only_file = false) const {
        for (const auto& cur_file : std::filesystem::recursive_directory_iterator(root_)) {
            if (cur_file.is_directory() && only_file)
                continue;
            if (deep_ != -1 && get_deep(cur_file) > deep_)
                continue;
            callback(cur_file.path().lexically_normal());
        }

    }
};


NAMESPACE_END
