/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/16 17:02:37
********************************************************************************/

module;
#include <filesystem>
#include <functional>
#include <algorithm>
export module modforge.directory;

export class Directory {
    std::filesystem::path root_path_ = ".";
    std::function<void(std::filesystem::path)> batching_func_;


public:
    bool only_batching_file = false;
    size_t limit_deep = 0;
    size_t limit_times = 0;

    // @TODO 计算两个路径之间的差值？
    // @TODO 路径不一定谁的路径更长
    // @TODO 返回两个路径之间的距离（如果不在同一条线上）？
    [[nodiscard]]
    auto get_deep(const std::filesystem::path& cur_path) const {
        auto path = cur_path.lexically_relative(root_path_).generic_string();
        return std::ranges::count(path.begin(), path.end(), '/') + 1;
    }

    explicit Directory(const std::filesystem::path &root_path) : root_path_(root_path) {  }

    template <typename Func>
    Directory(const std::filesystem::path &root_path,Func func)
            : root_path_(root_path) ,batching_func_(func) {  }

    template <typename Func>
    void set_batching_function(Func func) {
        batching_func_ = func;
    }

    [[nodiscard]]
    size_t count() const {
        return std::ranges::count_if(std::filesystem::recursive_directory_iterator(root_path_),[](const std::filesystem::path &path) {
            return !is_directory(path);
        });
    }

    void operator() () const {
        if (!std::filesystem::exists(root_path_))
            return;

        for (size_t times = 0 ;const auto& file : std::filesystem::recursive_directory_iterator(root_path_)) {

            if (limit_deep && get_deep(file) > limit_deep)
                continue;
            if (only_batching_file && is_directory(file))
                continue;

            batching_func_(file);

            if (limit_times && ++times >= limit_times)
                return;
        }
    }

};