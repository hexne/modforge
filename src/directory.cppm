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

    /** @brief 构造目录遍历器
     *  @param root 根目录路径
     *  @param deep 递归深度上限，-1 表示不限
     */
    explicit Directory(const std::filesystem::path &root, int deep = -1) : root_(root), deep_(deep) {  }

    /** @brief 查找根目录下所有匹配后缀的文件
     *  @param extent 后缀集合，';' 分隔，如 ".cpp;.h"
     *  @param only_name 仅返回文件名（无路径无后缀）
     *  @param only_file 仅返回文件（跳过文件夹）
     */
    std::vector<std::string> files(std::string extent, bool only_name = false, bool only_file = false) {
        auto extents = StringUtils::split(extent, ';');
        std::erase_if(extents, [](const std::string& e) { return e.empty(); });

        std::vector<std::string> ret;
        auto check_extent = [&](std::string path) -> bool {
            return std::ranges::any_of(extents, [&](const std::string &ext) {
                return path.ends_with(ext);
            });
        };

        traverser_directory([&](std::filesystem::path path) {
            if (!check_extent(path_string(path)))
                return;

            if (only_name)
                ret.push_back(path_string(path.filename()));
            else if (!only_name)
                ret.push_back(path_string(path.lexically_normal()));
        }, only_file);
        return ret;
    }

    /** @brief 递归遍历目录树，对每个条目调用 callback
     *  @param callback 每个条目的回调，接收归一化后的路径
     *  @param only_file true 时跳过文件夹
     */
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
