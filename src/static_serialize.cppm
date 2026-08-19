/********************************************************************************
* @Author : hexne
* @Date   : 2026/02/20 21:57:13
********************************************************************************/
export module modforge.static_serialize;
import std;

#define NAMESPACE_BEGIN namespace modforge {
#define NAMESPACE_END }


export NAMESPACE_BEGIN


// 类型判断
template <typename T>
concept is_base_type = std::is_integral_v<T>
                    || std::is_floating_point_v<T>
                    || std::is_same_v<T, bool>
                    || std::is_same_v<T, char>
                    || std::is_same_v<T, unsigned char>;

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
template <typename T, std::size_t N>
struct is_std_type<std::array<T, N>> : std::true_type {};
template <>
struct is_std_type<std::string> : std::true_type {};

// byte 相关函数
enum class ByteSize : std::uint8_t {
    byte1, byte4, byte8, byte16, byte32, byte64, dynamic
};
template <typename T>
constexpr ByteSize type_to_byte_size() {
    if constexpr (sizeof(T) == 1)
        return ByteSize::byte1;
    else if constexpr (sizeof(T) == 4)
        return ByteSize::byte4;
    else if constexpr (sizeof(T) == 8)
        return ByteSize::byte8;
    else if constexpr (sizeof(T) == 16)
        return ByteSize::byte16;
    else if constexpr (sizeof(T) == 32)
        return ByteSize::byte32;
    else if constexpr (sizeof(T) == 64)
        return ByteSize::byte64;
    else
        return ByteSize::dynamic;
}
constexpr std::size_t byte_size_to_size(ByteSize size) {
    switch (size) {
    case ByteSize::byte1: return 1;
    case ByteSize::byte4: return 4;
    case ByteSize::byte8: return 8;
    case ByteSize::byte16: return 16;
    case ByteSize::byte32: return 32;
    case ByteSize::byte64: return 64;
    }
    throw std::runtime_error("error ByteSize value");
}

// Archive 相关类
template <typename Derived>
struct Archive {
    Derived& self(this auto &self) {
        return static_cast<Derived&>(self);
    }

    void read_count_byte_size() {
        self().read_count_byte_size();
    }
    void skip_count_byte_size() {
        self().skip_count_byte_size();
    }

    void write_count_byte_size() {
        self().write_count_byte_size();
    }

    template<typename T>
    void write(const T &value) {
        self().write(value);
    }

    template<typename T>
    bool read(T &value) {
        return self().read(value);
    }

    void write_byte_size(ByteSize byte_size, std::size_t size = 0) {
        write(byte_size);
        if (byte_size == ByteSize::dynamic)
            write(size);
    }
    std::size_t read_byte_size() {
        ByteSize byte_size{};
        read(byte_size);
        std::size_t size{};
        if (byte_size == ByteSize::dynamic)
            read(size);
        else
            size = byte_size_to_size(byte_size);
        return size;
    }

    void write_id(std::size_t id) {
        write(id);
    }
    bool read_id(std::size_t &size) {
        return read(size);
    }


    // 在外层调用查找id
    //      => 如果找到就
    template <typename T> requires is_base_type<T>
    void serialize(std::size_t id, const T &obj) {
        write_id(id);
        write_byte_size(type_to_byte_size<T>(), sizeof(T));
        write(obj);
    }

    template <typename T> requires is_base_type<T>
    void deserialize(T &obj) {
        // id在外面会读取, 此处不能重复读取
        auto byte_size = read_byte_size();
        read(obj);
    }

    template <typename T> requires is_base_type<T>
    void serialize(std::size_t id, const std::vector<T> &obj) {
        write_id(id);
        write_byte_size(ByteSize::dynamic, sizeof(T) * obj.size());
        write(obj.size());
        for (const auto &val : obj) {
            write(val);
        }
    }

    template <typename T> requires is_base_type<T>
    void deserialize(std::vector<T> &obj) {
        auto byte_size = read_byte_size();
        std::size_t size;
        read(size);
        obj.resize(size);
        for (auto &val : obj) {
            read(val);
        }
    }

    void serialize(std::size_t id, const std::string &obj) {
        write_id(id);
        write_byte_size(ByteSize::dynamic, obj.size());
        write(obj.size());
        for (const auto &ch : obj) {
            write(ch);
        }
    }
    void deserialize(std::string &obj) {
        auto byte_size = read_byte_size();
        std::size_t size;
        read(size);
        obj.resize(size);
        for (auto &ch : obj) {
            read(ch);
        }
    }

    void skip() {
        auto byte_size = read_byte_size();
        self().skip(byte_size);
    }
};

class ArchivePointer : public Archive<ArchivePointer> {
    unsigned char *base{};
    unsigned char *p{};
    std::size_t count_byte_size{};
public:
    explicit ArchivePointer(unsigned char *buffer) : base(buffer), p(buffer) {  }

    // 反序列化时初始化设置
    void read_count_byte_size() {
        std::memcpy(&count_byte_size, p, sizeof(count_byte_size));
        p += sizeof(count_byte_size);
    }

    // 序列化时使用
    void skip_count_byte_size() {
        skip(sizeof(std::size_t));
    }

    void write_count_byte_size() {
        std::size_t total_size = static_cast<std::size_t>(p - base);
        std::memcpy(base, &total_size, sizeof(std::size_t));
    }


    template<typename T>
    void write(const T &value) {
        std::memcpy(p, &value, sizeof(T));
        p += sizeof(T);
    }

    template<typename T>
    bool read(T &value) {
        if (static_cast<std::size_t>(p - base) + sizeof(T) > count_byte_size)
            return false;
        std::memcpy(&value, p, sizeof(T));
        p += sizeof(T);
        return true;
    }

    using Archive::skip;
    void skip(std::size_t size) {
        p += size;
    }

};

class ArchiveStream : public Archive<ArchiveStream>
{
    std::fstream* file{};

public:

    explicit ArchiveStream(std::fstream& file)
        : file(&file)
    {
    }


    void read_count_byte_size()
    {
        file->seekg(sizeof(std::size_t), std::ios::cur);
    }


    void skip_count_byte_size()
    {
        file->seekp(sizeof(std::size_t), std::ios::cur);
    }


    void write_count_byte_size()
    {
        auto end = file->tellp();

        std::size_t total_size =
            static_cast<std::size_t>(end);

        file->seekp(0);

        write(total_size);

        file->seekp(end);
    }


    template<typename T>
    void write(const T& value)
    {
        file->write(
            reinterpret_cast<const char*>(&value),
            sizeof(T)
        );
    }


    template<typename T>
    bool read(T& value)
    {
        file->read(
            reinterpret_cast<char*>(&value),
            sizeof(T)
        );

        return static_cast<bool>(*file);
    }


    using Archive::skip;
    void skip(std::size_t size)
    {
        file->seekg(size, std::ios::cur);
    }
};

// 支持的序列化标记
namespace serialize_flag {
    struct ignore {} ignore;
    struct id {int val;};
}


// id相关
consteval std::size_t hash_string(std::string_view str) {
    std::size_t hash = 14695981039346656037ull;
    for (char c : str) {
        hash ^= static_cast<std::size_t>(c);
        hash *= 1099511628211ull;
    }
    return hash;
}
consteval std::size_t info_to_hash(std::string_view type, std::string_view name) {
    std::size_t hash = 14695981039346656037ull;

    auto add = [&](std::string_view str) {
        for(char c : str) {
            hash ^= static_cast<std::size_t>(c);
            hash *= 1099511628211ull;
        }
    };

    add(type);
    add(name);

    return hash;
}
consteval std::optional<std::meta::info> has_annotation(std::meta::info entity, std::meta::info type) {
    auto annotations = std::meta::annotations_of_with_type(entity, type);
    if (annotations.empty()) {
        return {};
    } else if (annotations.size() == 1) {
        return annotations.front();
    } else {
        throw "too many annotations!";
    }
}
template <std::meta::info info>
constexpr std::size_t info_to_id(auto &obj) {
    std::size_t id{};
    // 如果指定了id
    if constexpr (constexpr auto anns = has_annotation(info, ^^struct serialize_flag::id); anns) {
        constexpr auto tmp = *anns;
        id = std::meta::extract<struct serialize_flag::id>(tmp).val;
    }
    // 自动生成id
    else {
        constexpr auto type_string = std::meta::display_string_of(std::meta::type_of(^^obj));
        constexpr auto obj_string = std::meta::display_string_of(info);
        id = info_to_hash(type_string, obj_string);
    }
    return id;
}



// 补充的类型判断
template<typename T, typename Archive>
concept have_serialize_func = requires(T &obj, Archive &ar) {
    obj.serialize(ar);
    obj.deserialize(ar);
};

template<typename T, typename Archive>
concept can_serialize = requires(T& obj, Archive &ar) {
    serialize(obj, ar);
    deserialize(obj, ar);
};


// 立即序列化


// std迭代序列化




// 反射一个类
template <std::meta::info info, typename Callback>
void active_member(auto &obj, Callback &&callback) {
    // 忽略掉标记为 ignore 的字段
    if constexpr (constexpr auto anns = has_annotation(info, ^^struct serialize_flag::ignore); anns)
        return;
    using T = std::remove_cvref_t<decltype(obj)>;

    if constexpr (is_base_type<T>
                || is_pointer_type<T>::value
                || have_serialize_func<T, ArchivePointer>
                || have_serialize_func<T, ArchiveStream>
                || can_serialize<T, ArchivePointer>
                || can_serialize<T, ArchiveStream>
                || is_std_type<T>::value) {
        callback.template operator()<info>(obj);
    }
    // 普通类没有内置支持，自动递归序列化，作为一个复合体处理
    else if constexpr (std::is_class_v<T>) {
        static constexpr auto bases_class_info = std::define_static_array(
            std::meta::bases_of(^^T, std::meta::access_context::unchecked())
        );
        template for (constexpr auto base_class_info : bases_class_info) {
            active_member<base_class_info>(obj.[:base_class_info:], callback);
        }

        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())
        );
        template for (constexpr auto cur_info : members) {
            active_member<cur_info>(obj.[:cur_info:], callback);
        }

        static constexpr auto static_members = std::define_static_array(
            std::meta::static_data_members_of(^^T, std::meta::access_context::unchecked())
        );
        template for (constexpr auto cur_info : static_members) {
            active_member<cur_info>(obj.[:cur_info:], callback);
        }
        return;
    }
}

template<typename T, typename Callback>
bool find_by_id_callback(std::size_t id,T &obj, Callback&& callback) {
    bool found = false;
    active_member<^^T>(obj, [&]<std::meta::info info>(auto &member) {
            if (found)
                return;
            constexpr auto member_id = info_to_id<info>(member);
            if (member_id == id) {
                callback(member);
                found = true;
            }
        }
    );
    return found;
}

template <typename T, typename Archive>
void serialize_impl(T &obj, Archive &ar) {
    ar.skip_count_byte_size();
    active_member<^^T>(obj,[&]<std::meta::info info>(auto &obj) {
        constexpr std::size_t id = info_to_id<info>(obj);
        ar.serialize(id, obj);
    });
    ar.write_count_byte_size();
}

template <typename T, typename Archive>
void deserialize_impl(T &obj, Archive &ar) {

    ar.read_count_byte_size();
    std::size_t id{};
    while (ar.read_id(id)) {
        bool found_flag = find_by_id_callback(id, obj, [&](auto &member) {
            ar.deserialize(member);
        });
        if (!found_flag) {
            ar.skip();
            std::println("id {} 不存在，跳过", id);
        }
    }

}

struct serialize_cpo {
    template <typename T, typename Archive>
    constexpr auto operator()(T&obj, Archive &ar) const
        noexcept (noexcept(serialize(obj, ar)))
        -> decltype(serialize(obj, ar)) {
            return serialize(obj, ar);
    }
    template <typename T, typename Archive>
    constexpr auto operator()(T&obj, Archive &ar) {
        return serialize_impl(obj, ar);
    }
};


struct deserialize_cpo {
    template <typename T, typename Archive>
    constexpr auto operator()(T&obj, Archive &ar) const
        noexcept (noexcept(deserialize(obj, ar)))
        -> decltype(deserialize(obj, ar)) {
            return deserialize(obj, ar);
        }
    template <typename T, typename Archive>
    constexpr auto operator()(T&obj, Archive &ar) {
        return deserialize_impl(obj, ar);
    }
};
inline serialize_cpo serialize{};
inline deserialize_cpo deserialize{};

NAMESPACE_END

