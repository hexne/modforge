/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/02/17 15:30
*******************************************************************************/

module;
#include <map>
#include <iostream>
#include <string>
#include <vector>

#include <json/json.h>
#include "tools.h"
export module Json;

export
NAMESPACE_BEGIN(nl)
class JsonValue {
    std::string value_;
    std::vector<std::string> value_arr_;
    bool is_array_{};
    JsonValue(const std::string &value) : value_(value) {  }
    JsonValue(const std::vector<std::string> &values) : value_arr_(values) ,is_array_(true) {  }

    friend std::ostream &operator<<(std::ostream &os, const JsonValue &json_value) {
        if (json_value.is_array_) {
            os << "[\n";

            os << "]\n";
        }
        else {

        }

    }
};

class Json {
    std::map<std::string, JsonValue> map_;
public:
    Json(const std::string &str) {


    }
    void add_pair(const std::string &key, const JsonValue &value) {
        map_[key] = value;
    }
    friend std::ostream &operator << (std::ostream &os, const Json &json) {
        os << "{\n";
        size_t tab{};
        for (const auto &pair : json.map_) {
            os << pair.first << " : " << pair.second;


        }


        os << "}\n";
        return os;
    }

};

NAMESPACE_END(nl)