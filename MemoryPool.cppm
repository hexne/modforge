/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/5/28
*******************************************************************************/

module;
#include <algorithm>
#include <functional>
#include <memory>
#include <opencv2/imgproc.hpp>
#include <ranges>
#include <set>
#include <vector>

#include "tools.h"
export module MemoryPool;

export
NAMESPACE_BEGIN(nl)
template <typename T>
class MemoryPool {
    struct MemoryNode {
        T *addr;
        size_t count;

        bool operator < (const MemoryNode &other) const {
            return count < other.count;
        }
    };

    T *addr_{};
    size_t count_{};
    std::vector<bool> flag_;
    std::multiset<MemoryNode> set_;

    // 最佳适应算法进行分配内存
    T *find_malloc_addr(size_t count) {
        auto find = set_.lower_bound({nullptr,count});
        return find != set_.end() ? find->addr : nullptr;
    }
    void merge_free(T *addr,size_t count) {
        int cur_index = addr - addr_;
        int back_index = cur_index + count;
        int tmp_index = cur_index - 1;

        auto back_ptr = addr + count;
        // 后面需要合并
        if (back_ptr != addr + count_ && !flag_[back_index]) {
            auto back_it = std::ranges::find_if(set_,[back_ptr] (const MemoryNode &node){
                return node.addr == back_ptr;
            });
            auto [back_addr,back_count] = *back_it;
            set_.erase(back_it);
            set_.insert({addr,count + back_count});
        }

        // 首先先从当前地方向前找到第一个flag_[index]为true的，并+1,得到最后一个值为false的
        while (tmp_index >= 0 && tmp_index --) {
            if (flag_[tmp_index] == true)
                break;
        }
        int front_index = ++ tmp_index;
        auto front_ptr = addr - (cur_index - front_index);

        // 前面的需要合并
        if (front_index != cur_index && !flag_[front_index]) {
            auto front_it = std::ranges::find_if(set_,[front_ptr] (const MemoryNode &node){
                return node.addr == front_ptr;
            });
            auto [front_addr,front_count] = *front_it;
            set_.erase(front_it);
            set_.insert({front_addr,count + front_count});
        }
    }
    void update_memory_pool(T *addr, size_t count, bool is_malloc) {
        for (long tmp = count - 1; tmp >= 0; -- tmp)
            flag_[addr - addr_ + count] = is_malloc;

        if (is_malloc) {
            auto it = std::ranges::find_if(set_, [addr](const MemoryNode &node) {
                return node.addr == addr;
            });
            auto [addr_back,count_back] = *it;
            set_.erase(it);
            set_.insert({addr + count, count_back - count});
        }
        else {
            merge_free(addr,count);
        }
    }
public:
    explicit MemoryPool(const size_t count) : count_(count) {
        flag_.resize(count_);
        addr_ = static_cast<T *>(::malloc(count_ * sizeof(T)));
        set_.insert({addr_,count});
    }
    ~MemoryPool() {
        ::free(addr_);
        addr_ = nullptr;
    }
    static std::unique_ptr<MemoryPool>& GetInstance() {
        static std::unique_ptr<MemoryPool> instance;
        if (!instance)
            instance = std::make_unique<MemoryPool>();
        return instance;
    }

    T *malloc() {
        T *ret = find_malloc_addr(1);
        if (ret == nullptr)
            throw std::bad_alloc();
        ::new (ret) T;
        update_memory_pool(ret,1,true);
        return ret;
    }
    // free中应调用 delete
    void free(T *addr) {
        (*addr).~T();
        update_memory_pool(addr,1,false);
    }

    std::tuple<T *,size_t> malloc(size_t count) {
        auto ret = find_malloc_addr(count);

        for (int i = 0;i < count; ++i)
            new (ret + i) T();

        update_memory_pool(ret, count, true);
        return {ret, count};
    }
    void free(std::tuple<T *,size_t> addr_info) {
        auto [addr, count] = addr_info;

        for (size_t i = 0;i < count; ++i)
            (addr + i)->~T();

        update_memory_pool(addr, count, false);
    }
};

template <>
class MemoryPool<void> {  };
NAMESPACE_END(nl)
