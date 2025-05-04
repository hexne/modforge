/*******************************************************************************
 * @Author : hexne
 * @Data   : 2023/05/30 20:20
*******************************************************************************/

module;
#include <vector>
#include <thread>
#include <algorithm>
#include <queue>
#include <functional>
#include <mutex>
#include <future>
#include <condition_variable>
#include "tools.h"
export module ThreadPool;


export
NAMESPACE_BEGIN(nl)
class ThreadPool {
    std::vector<std::thread> work_;
    std::queue<std::function<void()>> tasks_; // 任务队列
    std::condition_variable cv_;
    std::mutex _mutex;

    bool stop_ = false;
    size_t pool_size_;
public:
    ThreadPool(size_t size) : pool_size_(size) {
        for (int i = 0; i < size; i++) {
            work_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        //临界区开始 等待线程池的销毁或者任务
                        std::unique_lock<std::mutex> lock(_mutex);
                        cv_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); }); //条件变量的wait函数 防止虚假唤醒

                        if (this->stop_ && this->tasks_.empty()) {
                            return; //线程池销毁 跳出循环
                        }

                        task = std::move(this->tasks_.front()); //将队列的头部移动给task
                        this->tasks_.pop(); //记得出队列
                    }
                    task(); //执行任务
                }
            });
        }
    }


    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            stop_ = true;
        }
        cv_.notify_all();   //通知所有的线程退出循环
        for_each(work_.begin(), work_.end(), std::mem_fn(&std::thread::join)); //等待
    }

    template<typename Func, typename ... Args>
    decltype(auto) Enqueue(Func&& func, Args&& ... args) {
        using RetType = typename std::invoke_result<Func,Args...>::type; //获取返回值类型

        auto task = std::make_shared< std::packaged_task<RetType()> >(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...) //通过bind和完美转发将函数和参数绑定
        );

        std::future<RetType> res = task->get_future();

        {
            //临界区开始 锁任务队列
            std::unique_lock<std::mutex> lock(_mutex);

            //如果已经将线程池析构,就不允许再入队列
            if (stop_)
                throw std::runtime_error("线程池停止");

            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one(); // 唤起一个线程
        return res;
    }
};

NAMESPACE_END(nl)

