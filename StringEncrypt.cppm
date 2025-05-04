/*******************************************************************************
* @Author : hexne
* @Data   : 2022/05/09 11:29:07
*******************************************************************************/

module;
#include <iostream>
#include <string>
#include "tools.h"
export module StringEncrypt;


export
NAMESPACE_BEGIN(nl)

constexpr size_t GetKey() {
#define TO_INT(ch1,ch2) (((ch1)-'0')*10 + ((ch2)-'0'))
	// 00:00:00\
		0 1 : 3 4 : 6 7

	return TO_INT(__TIME__[6], __TIME__[7])
		+ TO_INT(__TIME__[3], __TIME__[4])
		+ TO_INT(__TIME__[0], __TIME__[1]);

#undef TO_INT
}

constexpr char Encrypt(const unsigned char ch, const size_t key) {
	return ch ^ key;
}
char Decrypt(const unsigned char ch, const size_t key) {
	return ch ^ key;
}

template<std::size_t N>
struct StringEncrypt {
	char s[N]{};
	size_t key = GetKey();
	constexpr explicit StringEncrypt(const char (&str)[N]) {
		for (int i = 0; i < N; ++i) {
			s[i] = Encrypt(str[i],key);
		}
	};

	explicit operator std::string () const {
		std::string ret(s);
		for (int i = 0; i < N; ++i) {
			ret[i] = Decrypt(ret[i], key);
		}
		return ret;
	}
};

NAMESPACE_END(nl)


export NAMESPACE_BEGIN(nl::string_encrypt_literals)

template<nl::StringEncrypt se>
std::string operator""_crypt() {
	return se;
}
NAMESPACE_END(nl::string_encrypt_literals)
