/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/30 23:44:54
********************************************************************************/

module;
export module modforge.id_generator;
import std;

NAMESPACE_BEGIN
export class IDGenerator {
    std::atomic<int> id{1};
public:
    int next() {
        return id.fetch_add(1, std::memory_order_relaxed);
    }
};
NAMESPACE_END