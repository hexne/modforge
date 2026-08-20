/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/20 11:41:31
********************************************************************************/

module;
export module modforge.signal;

import std;

NAMESPACE_BEGIN

struct Callback {
    int id{};
    std::function<void()> callback;
    int count{};
    bool connected = true;
};

export class Signal {
    int id_create_{};

    std::list<Callback> callbacks_;

    int connect_impl(std::function<void()> callback, int n) {
        const int id = id_create_++;

        callbacks_.push_front(Callback{
            .id = id,
            .callback = std::move(callback),
            .count = n,
            .connected = true
        });

        return id;
    }
public:
    void connect(std::function<void()> callback) {
        connect_impl(std::move(callback), -1);
    }
    void connect_once(std::function<void()> callback) {
        connect_impl(std::move(callback), 1);
    }
    void connect_n(std::function<void()> callback, int n) {
        connect_impl(std::move(callback), n);
    }

    void disconnect(int id) {
        auto it = std::ranges::find_if(callbacks_, [id](const Callback &c) {
            return id == c.id;
        });

        if (it != callbacks_.end())
            it->connected = false;
    }

    void emit() {
        for (auto & callback : callbacks_) {
            if (!callback.connected)
                continue;

            callback.callback();
            if (callback.count == -1)
                continue;

            callback.count--;
            if (callback.count <= 0)
                callback.connected = false;
        }

        std::erase_if(callbacks_, [](const Callback &c) {
            return !c.connected;
        });
    }
};

NAMESPACE_END

