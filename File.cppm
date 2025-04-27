/*******************************************************************************
 * @Author  : hexne
 * @Data    : 2024/5/18 0:8
 * @warning : 使用vs时，由于wstring_convert 和convert_utf8已经已经被C++17标记为弃用
 *           因此需要添加宏：_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
*******************************************************************************/
module;
#include <algorithm>
#include <iterator>
#include <string>
#include <codecvt>
#include <fstream>
#include <filesystem>
#include <functional>
#include <variant>
#include <optional>
#include "tools.h"
export module File;


#if defined(_WIN32) || defined(_WIN64)
const char* const GbkLocalString = "zh_CN";
#else
const char* const GbkLocalString = "zh_CN.GBK";
#endif


bool DetectUtf8Coding(const std::string_view text) {

    unsigned int byte_count = 0;//UFT8可用1-6个字节编码,ASCII用一个字节

    bool all_ascii = true;
    for (const unsigned char ch : text) {
        //判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx
        if (byte_count == 0 && (ch & 0x80) != 0) {
            all_ascii = false;
        }
        if (byte_count == 0) {
            //如果不是ASCII码,应该是多字节符,计算字节数
            if (ch >= 0x80) {
                if (ch >= 0xFC && ch <= 0xFD) {
                    byte_count = 6;
                }
                else if (ch >= 0xF8) {
                    byte_count = 5;
                }
                else if (ch >= 0xF0) {
                    byte_count = 4;
                }
                else if (ch >= 0xE0) {
                    byte_count = 3;
                }
                else if (ch >= 0xC0) {
                    byte_count = 2;
                }
                else {
                    return false;
                }
                byte_count--;
            }
        }
        else {
            //多字节符的非首字节,应为 10xxxxxx
            if ((ch & 0xC0) != 0x80) {
                return false;
            }
            //减到为零为止
            byte_count--;
        }

    }
    //违返UTF8编码规则
    if (byte_count != 0) {
        return false;
    }
    if (all_ascii) { //如果全部都是ASCII, 也是UTF8
        return true;
    }
    return true;
}

bool DetectGBKCoding(const std::string_view text) {
    unsigned int byte_count = 0;//GBK可用1-2个字节编码,中文两个 ,英文一个
    bool all_ascii = true; //如果全部都是ASCII,
    for (const unsigned char ch : text) {
        if (( ch & 0x80) != 0 && byte_count == 0) {// 判断是否ASCII编码,如果不是,说明有可能是GBK
            all_ascii = false;
        }
        if (byte_count == 0) {
            if ( ch >= 0x80) {
                if ( ch >= 0x81 &&  ch <= 0xFE) {
                    byte_count = +2;
                }
                else {
                    return false;
                }
                byte_count--;
            }
        }
        else {
            if ( ch < 0x40 ||  ch>0xFE) {
                return false;
            }
            byte_count--;
        }//else end

    }
    if (byte_count != 0) {   //违返规则
        return false;
    }
    if (all_ascii) { //如果全部都是ASCII, 也是GBK
        return true;
    }
    return true;
}

class codecvt_gbk final : public std::codecvt_byname<wchar_t, char, std::mbstate_t> {
public:
    codecvt_gbk() : codecvt_byname(GbkLocalString) {  }
};

static std::wstring_convert<codecvt_gbk> gbk_convert;
static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;


/**
 * @brief : 获取相对路径层数
 * @param root_path : 根路径
 * @param cur_path : 当前路径
 * @return : 当前路径相对于根路径的层数
**/
auto GetDeep(const std::filesystem::path& root_path, const std::filesystem::path& cur_path) {
    auto path = cur_path.lexically_relative(root_path).generic_string();
    return std::ranges::count(path.begin(), path.end(), '/') + 1;
}

export
NAMESPACE_BEGIN(nl)

/*******************************************************************************
 * string <==> wstring
*******************************************************************************/
std::string WStringToString(const std::wstring_view wstring) {
    return converter.to_bytes({wstring.data(), wstring.size()});
}
std::wstring StringToWString(const std::string &str) {
    return converter.from_bytes(str);
}

/*******************************************************************************
 * gbk(wstring) <==> utf8
 * gbk(string)  <==> utf8
*******************************************************************************/
std::string GBKToUTF8(const std::wstring &str) {
    return WStringToString(str);
}
std::string GBKToUTF8(const std::string& str) {
    return converter.to_bytes(gbk_convert.from_bytes(str));
}
std::wstring UTF8ToWstringGBK(const std::string &str) {
    return StringToWString(str);
}
std::string UTF8ToStringGBK(const std::string& str) {
    return gbk_convert.to_bytes(converter.from_bytes(str));
}

class Encoding {
    enum class EncodingType {
        None, GBK, UTF8, ERROR
    };

    friend std::ostream& operator << (std::ostream & out, const EncodingType & encoding) {
        switch (encoding) {
        case Encoding::EncodingType::GBK:
            out << "GBK";
            break;
        case Encoding::EncodingType::UTF8:
            out << "UTF8";
            break;
        case Encoding::EncodingType::ERROR:
            out << "Error";
            break;
        default:
            break;
        }
        return out;

    }

    EncodingType encoding_type_ = EncodingType::None;

    std::filesystem::path file_path_;
    std::variant<std::string,std::wstring> content_;


public:
    using EncodingType = EncodingType;

    explicit Encoding(const std::filesystem::path &file_path) : file_path_(std::move(file_path)) {
        std::ifstream file(file_path_);
        std::get<std::string>(content_) = std::string(std::istreambuf_iterator(file),std::istreambuf_iterator<char>());
        encoding_type_ = detect_encoding(std::get<std::string>(content_));
    }
    explicit Encoding(const std::string_view string) {
        std::get<std::string>(content_) = string;
        encoding_type_ = detect_encoding(std::get<std::string>(content_));
    }
    explicit Encoding(const std::wstring_view wstring) {
        std::get<std::wstring>(content_) = wstring;
        encoding_type_ = detect_encoding(wstring_to_string(std::get<std::wstring>(content_)));
    }

    void save(const std::filesystem::path &path) const {
        if (encoding_type_ == EncodingType::GBK) {
            std::wofstream out_file(path);
            out_file.imbue(std::locale(GbkLocalString));
            out_file << std::get<std::wstring>(content_);
        }
        else if (encoding_type_ == EncodingType::UTF8) {
            std::ofstream out_file(path);
            out_file << std::get<std::string>(content_);
        }
    }

    [[nodiscard]]
    EncodingType encoding() const {
        return encoding_type_;
    }

    static EncodingType detect_encoding(const std::string_view content) {
        if (DetectUtf8Coding(content) == true)
            return EncodingType::UTF8;
        if (DetectGBKCoding(content) == true)
            return EncodingType::GBK;
        return EncodingType::ERROR;
    }
    static std::string wstring_to_string(const std::wstring_view wstring) {
        return converter.to_bytes({wstring.data(), wstring.size()});
    }
    static std::wstring string_to_wstring(const std::string &str) {
        return converter.from_bytes(str);
    }
    std::string to_utf8() {
        if (encoding_type_ == EncodingType::UTF8)
            ;
        if (encoding_type_ == EncodingType::GBK)
            std::get<std::string>(content_) = GBKToUTF8(std::get<std::wstring>(content_));
        encoding_type_ = EncodingType::UTF8;
        return std::get<std::string>(content_);
    }
    std::wstring to_gbk() {
        if (encoding_type_ == EncodingType::UTF8)
            std::get<std::wstring>(content_) = UTF8ToWstringGBK(std::get<std::string>(content_));
        if (encoding_type_ == EncodingType::GBK)
            ;
        encoding_type_ = EncodingType::GBK;
        return std::get<std::wstring>(content_);
    }

    std::optional<std::variant<std::string,std::wstring>> operator()() {
        if (encoding_type_ == EncodingType::UTF8)
            return std::get<std::string>(content_);
        if (encoding_type_ == EncodingType::GBK)
            return std::get<std::wstring>(content_);
        return std::nullopt;
    }


};

class Directory {
    std::filesystem::path root_path_ = ".";
    std::function<void(std::filesystem::path)> batching_func_;

    [[nodiscard]]
    auto get_deep(const std::filesystem::path& cur_path) const {
        return GetDeep(root_path_, cur_path);
    }

public:
    bool only_batching_file = false;
    size_t deep = 0;
    size_t limit_times = 0;


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

            if (deep && get_deep(file) > deep)
                continue;
            if (only_batching_file && is_directory(file))
                continue;

            batching_func_(file);

            if (limit_times && ++times >= limit_times)
                return;
        }
    }

};

NAMESPACE_END(nl)


