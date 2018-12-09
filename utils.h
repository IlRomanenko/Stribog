#pragma once

#include <cstdlib>
#include <cstdint>
#include <string>
#include <iostream>
#include <cstring>
#include <algorithm>

using block_t = uint8_t *;

namespace utils {

template<typename T>
T parse_hex(const std::string &hex_str) {
    size_t size = std::min(hex_str.length(), sizeof(T) * 2);

    T result = 0;
    for (size_t i = 0; i < size; i++) {
        const auto &chr = (char) tolower(hex_str[i]);
        result <<= 4;
        if (isdigit(chr)) {
            result |= (uint8_t) (chr - '0');
        } else {
            result |= (uint8_t) (chr - 'a' + 10);
        }
    }
    return result;
}

template<typename T>
void print_hex(T value, std::string message = "", bool need_flush = true) {
    size_t size = sizeof(T);

    auto convert_to_hex = [](int x) {
        char chr;
        if (x < 10) {
            chr = (char) (x + '0');
        } else {
            chr = (char) (x + 'a' - 10);
        }
        return chr;
    };

    std::string result;
    for (size_t i = 0; i < 2 * size; i++) {
        auto byte = (uint8_t) value & 0b1111;
        value >>= 4;
        result += convert_to_hex(byte);
    }

    std::reverse(result.begin(), result.end());
    if (!message.empty()) {
        std::cout << message << "\t:\t";
    }
    std::cout << result << (need_flush ? "\n" : "");
}

template<typename T>
void print_hex_array(const T arr, const std::string &msg = "", int level = 0) {
    if (level == 0) {
        std::cout << msg << std::endl;
    }
    for (auto value : arr) {
        print_hex_array(value, "", level + 1);
        std::cout << std::endl;
    }
    std::cout << std::endl << std::endl;
}

template<>
void print_hex_array(const __uint128_t arr, const std::string &msg, int level) {
    std::string local_msg;
    for (int i = 0; i < level; i++) {
        local_msg += "\t";
    }
    print_hex(arr, local_msg, false);
}


void print_hex(const uint8_t* str, size_t size, const std::string& msg="") {
    if (!msg.empty()) {
        std::cout << msg << "\t:\t";
    }
    auto convert_to_hex = [](int x) {
        char chr;
        if (x < 10) {
            chr = (char) (x + '0');
        } else {
            chr = (char) (x + 'a' - 10);
        }
        return chr;
    };

    for (size_t i = 0; i < size; i++) {
        std::cout << convert_to_hex((int)str[i] >> 4) << convert_to_hex((int)str[i] % (1<<4));
    }
    std::cout << std::endl;
}

template<>
void print_hex_array(uint8_t* arr, const std::string &msg, int level) {
    if (!msg.empty()) {
        std::cout << msg << std::endl;
    }
    print_hex(arr, 64, "\t");
}

template<>
void print_hex_array(const uint8_t* arr, const std::string &msg, int level) {
    if (!msg.empty()) {
        std::cout << msg << std::endl;
    }
    print_hex(arr, 64, "\t");
}

template<typename T>
T convert(const uint8_t *data) {
    static const size_t size = sizeof(T);
    static uint8_t arr[size];
    memcpy(arr, data, size);
    for (auto i = 0; i < size / 2; i++) {
        std::swap(arr[i], arr[size - 1 - i]);
    }
    return *((T *) arr);
}

template<typename T>
uint8_t *deconvert(const T &value, uint8_t *data) {
    memcpy(data, (uint8_t *) (&value), sizeof(T));
    for (auto i = 0; i < sizeof(T) / 2; i++) {
        T obj = data[i];
        data[i] = data[sizeof(T) - 1 - i];
        data[sizeof(T) - 1 - i] = obj;
//        std::swap(data[i], data[sizeof(T) - 1 - i]);
    }
    return data;
}

template<size_t SIZE>
uint8_t *copy(const uint8_t *data) {
    auto copied = new uint8_t[SIZE];
    memcpy(copied, data, SIZE);
    return copied;
}

template<size_t SIZE>
inline const uint8_t *empty_block() {
    static auto block = new uint8_t[SIZE];
    static bool initialized = false;
    if (!initialized) {
        memset(block, 0, SIZE);
    }
    return block;
}

template<typename T, size_t SIZE>
inline const uint8_t *get_block_with_value(T value) {
    auto block = copy<SIZE>(empty_block<SIZE>());
    for (size_t i = SIZE; i > 0; i--) {
        block[i - 1] = (uint8_t)value;
        value >>= 8;
    }
    return block;
}

template<typename T>
inline T reverse_bytes(T value) {
    static auto memory = new uint8_t[sizeof(T)];
    memcpy(memory, (uint8_t*)(&value), sizeof(T));
    for (size_t i = 0; i < sizeof(T) / 2; i++) {
        std::swap(memory[i], memory[sizeof(T) - 1 - i]);
    }
    return *((T*)(memory));
}

};
//
//
//std::pair<uint8_t*, size_t> convert_str_to_bytes(const std::string& str) {
//    auto result = new uint8_t[str.length() * 2];
//    for (size_t i = 0; i < str.length(); i++) {
//        result[i * 2] = ((uint8_t)str[i]) >> 4;
//    }
//    return std::make_pair(result, str.length() * 2);
//}
//
//void print_hex(const uint8_t* str, int length, const std::string& msg="") {
//    std::cout.flags(std::ios_base::hex);
//    std::cout << msg;
//    for (int i = 0; i < length; i++) {
//        std::cout << ((int)str[i] >> 4) << ((int)str[i] % (1<<4));
//    }
//    std::cout << std::endl;
//}
//
//uint8_t* concat(const uint8_t* str, int length) {
//    auto result = new uint8_t[length / 2];
//    for (int i = 0; i < length / 2; i++) {
//        result[i] = (str[2 * i] << 4) | str[2 * i + 1];
//    }
//    return result;
//}
//
//uint8_t* parse_hex_string(const std::string& str) {
//    auto result = new uint8_t[str.length()];
//    char chr;
//    for (size_t i = 0; i < str.length(); i++) {
//        chr = (char)tolower(str[i]);
//        if (isdigit(chr)) {
//            result[i] = (uint8_t)(chr - '0');
//        } else {
//            result[i] = (uint8_t)(chr - 'a' + 10);
//        }
//    }
//    return concat(result, (int)str.length());
//}
//
//block_t copy(const block_t block, const size_t length = 16) {
//    auto result = new uint8_t[length];
//    memcpy(result + length - 16, block, 16);
//    return result + length - 16;
//}
//
//
//template <typename T>
//T* add2(T *block, T *a) {
//    for (int i = 0; i < 16 / sizeof(T); i++) {
//        block[i] ^= a[i];
//    }
//    return block;
//}
//
