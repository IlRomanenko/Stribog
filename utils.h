#pragma once

#include <cstdlib>
#include <cstdint>
#include <string>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>

using block_t = uint8_t *;

namespace utils {
    uint8_t* concat(const uint8_t* str, int length) {
        auto result = new uint8_t[length / 2];
        for (int i = 0; i < length / 2; i++) {
            result[i] = (str[2 * i] << 4) | str[2 * i + 1];
        }
        return result;
    }

    uint8_t* parse_hex_string(const std::string& str) {
        auto result = new uint8_t[str.length()];
        char chr;
        for (size_t i = 0; i < str.length(); i++) {
            chr = (char)tolower(str[i]);
            if (isdigit(chr)) {
                result[i] = (uint8_t)(chr - '0');
            } else {
                result[i] = (uint8_t)(chr - 'a' + 10);
            }
        }
        return concat(result, (int)str.length());
    }

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
        initialized = true;
    }
    return block;
}

template<typename T, size_t SIZE>
inline const uint8_t *get_block_with_value(T value) {
    static T prev_value;
    static auto block = copy<SIZE>(empty_block<SIZE>());

    if (value == prev_value) {
        return block;
    }
    prev_value = value;
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

std::vector<__uint128_t> right_shifting(const std::vector<__uint128_t> &message, size_t bits_length) {
    std::vector<__uint128_t> result;

    /**
     * xxxxxxxxxxx000000000
     * < payload >< shift >
     */
    auto shift_size = 128 - bits_length % 128;
    for (int i = (int) message.size() - 1; i >= 0; i--) {
        __uint128_t cur_part = message[i];
        if (i != (int) message.size() - 1) {
            cur_part >>= shift_size;
        }
        __uint128_t prev_part = 0;
        if (i > 0) {
            prev_part = message[i - 1] & (~((-1) << shift_size));
        }
        cur_part |= prev_part << (128 - shift_size);
        result.push_back(cur_part);
    }
    std::reverse(result.begin(), result.end());
    return result;
}

uint8_t * add_padding(const std::vector<__uint128_t> &message,
                                             size_t bits_length) {
    std::vector<__uint128_t> message_ = right_shifting(message, bits_length);

    if (bits_length % 512 != 0) {
        std::reverse(message_.begin(), message_.end());
        if (bits_length % 128 != 0) {
            auto value = message_.back();
            value |= (__int128_t) 1 << (bits_length % 128);
            message_[message_.size() - 1] = value;
        } else {
            message_.push_back({1});
            bits_length += 128;
        }

        auto need_blocks = (512 - bits_length) / 128;
        for (auto i = 0; i < need_blocks; i++) {
            message_.push_back({0});
        }
        std::reverse(message_.begin(), message_.end());
    }

    auto total_blocks = message_.size() / 4;

    auto * result = new uint8_t[total_blocks * 64];

    for (auto i = 0; i < total_blocks; i++) {
        for (int j = 0; j < 4; j++) {
            utils::deconvert<__uint128_t>(message_[i * 4 + j], result + j * 16 + i * 64);
        }
    }
    return result;
}

};