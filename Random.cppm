/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/03/21 21:57
*******************************************************************************/

module;
#include <random>
#include "tools.h"
export module Random;


export
NAMESPACE_BEGIN(nl)

namespace Random {
    std::random_device rd;
    std::mt19937 gen(rd());

    float get_random_float(float min, float max) {
        std::uniform_real_distribution<> dis(min, max);
        return dis(gen);
    }

}

NAMESPACE_END(nl)