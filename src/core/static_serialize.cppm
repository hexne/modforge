/********************************************************************************
* @Author : hexne
* @Date   : 2026/02/20 21:57:13
********************************************************************************/
module;
#include <meta>
export module modforge.static_serialize;
import std;
import std.compat;

template <typename T>
concept is_base_type = std::is_integral_v<T> or std::is_floating_point_v<T>;


template <typename T>
struct is_pointer_type : std::false_type {};
template <typename T>
struct is_pointer_type<T *> : std::true_type {};
template <typename T>
struct is_pointer_type<std::shared_ptr<T>> : std::true_type {};
template <typename T>
struct is_pointer_type<std::unique_ptr<T>> : std::true_type {};


template <typename>
struct is_std_type : std::false_type {};
template <typename T>
struct is_std_type<std::vector<T>> : std::true_type {};
template <typename T, size_t N>
struct is_std_type<std::array<T, N>> : std::true_type {};
template <>
struct is_std_type<std::string> : std::true_type {};


template <typename T>
concept can_serialize = requires(T t) {
    t.serialize(std::declval<std::fstream&>());
    t.deserialize(std::declval<std::fstream&>());
}
|| requires(T t) {
    serialize(std::declval<T&>(), std::declval<std::fstream&>());
    deserialize(std::declval<T&>(), std::declval<std::fstream&>());
};

// 对内置类型和std中的类型提供默认的序列化支持
template <typename T>
struct BuildInSerialize {};

template <typename T> requires is_base_type<T>
struct BuildInSerialize<T> {
    static void serialize(T &obj, std::fstream &file) {
        file.write(reinterpret_cast<const char*>(&obj), sizeof(T));
    }

    static void deserialize(T &obj, std::fstream &file) {
        file.read(reinterpret_cast<char* const>(&obj), sizeof(T));
    }

};

template <typename T>
struct BuildInSerialize<std::vector<T>> {
    static void serialize(std::vector<T>& vec, std::fstream& file) {
        size_t size = vec.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        for (auto& item : vec) {
            BuildInSerialize<T>::serialize(item, file);  // 明确调用
        }
    }

    static void deserialize(std::vector<T>& vec, std::fstream& file) {
        size_t size = 0;
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        vec.resize(size);
        for (auto& item : vec) {
            BuildInSerialize<T>::deserialize(item, file);  // 明确调用
        }
    }

};

template <typename T, size_t N>
struct BuildInSerialize<std::array<T, N>> {
    static void serialize(std::array<T, N>& arr, std::fstream& file) {
        for (auto& item : arr) {
            BuildInSerialize<T>::serialize(item, file);  // 明确调用
        }
    }

    static void deserialize(std::array<T, N>& arr, std::fstream& file) {
        for (auto& item : arr) {
            BuildInSerialize<T>::deserialize(item, file);  // 明确调用
        }
    }
};

template <>
struct BuildInSerialize<std::string> {
    static void serialize(std::string& obj, std::fstream& file) {
        size_t size = obj.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(obj.data(), static_cast<long>(size));
    }
    static void deserialize(std::string& obj, std::fstream& file) {
        size_t size = 0;
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        obj.resize(size);
        file.read(obj.data(), static_cast<long>(size));
    }

};

#define def(OP)\
export template <typename T>\
void OP(T& obj, std::fstream& stream) {\
    /* 如果是指针，提示不支持 */ \
    if constexpr (is_pointer_type<T>::value) {\
        static_assert(false, "不支持对指针进行序列化操作");\
    }\
    /* 使用内置支持的序列化函数 */ \
    else if constexpr (is_base_type<T> or is_std_type<T>::value) {\
        BuildInSerialize<T>::OP(obj, stream);\
    }\
    else if constexpr (can_serialize<T>) {\
        /* 按照下面的顺序判断可以避免递归 */ \
        /* 1，有成员函数，直接调用 */ \
        /* 2，没有成员函数，上面经过adl分析找到了更适配的函数，则调用 */ \
        if constexpr (requires (T t) { t.OP(std::declval<std::fstream &>()); } )\
            obj.OP(stream);\
        else\
            OP(obj, stream);\
    }\
    /* 如果是没有自定义序列化函数的类 */ \
    else if constexpr (std::is_class_v<T>) {\
        /* 递归每个父类 */ \
        static constexpr auto bases_class_info = std::define_static_array(\
            std::meta::bases_of(^^T, std::meta::access_context::unchecked())\
        );\
        template for (constexpr auto base_class_info : bases_class_info) {\
            auto &base = obj.[: base_class_info :];\
            OP<std::remove_cvref_t<decltype(base)>>(base, stream);\
        }\
        /* 递归当前类中的每个成员 */ \
        static constexpr auto members = std::define_static_array(\
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())\
        );\
        template for (constexpr auto info : members) {\
            OP<decltype(obj.[:info:])>(obj.[:info:], stream);\
        }\
        static constexpr auto static_members = std::define_static_array(\
            std::meta::static_data_members_of(^^T, std::meta::access_context::unchecked())\
        );\
        template for (constexpr auto info : static_members) {\
            OP<decltype(obj.[:info:])>(obj.[:info:], stream);\
        }\
    }\
    else {\
        static_assert(false, "error");\
    }\
}

NAMESPACE_BEGIN
def(serialize);
def(deserialize);
NAMESPACE_END