#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "constants.h"

using __uint512_t = std::vector<__uint128_t>;

class Stribog_hash {

    uint64_t linear_transformation(const uint64_t &value) const {
        const auto &transform_matrix = constants::linear_transformation::get_linear_matrix();

        uint64_t result = 0;
        for (int i = 0; i < 64; i++) {
            if ((value >> i) & 1) {
                result ^= transform_matrix[i];
            }
        }
        return result;
    }

    __uint128_t pi_transformation(const __uint128_t &value) const {
        const auto &pi = constants::pi_transformation::pi;

        __uint128_t result = 0;
        __uint128_t mask = 0b11111111;
        size_t offset = 0;
        for (int i = 0; i < 16; i++) {
            auto byte = (uint8_t) ((value & mask) >> offset);
            result |= (__uint128_t)pi[byte] << offset;
            mask <<= 8;
            offset += 8;
        }

        return result;
    }

    __uint512_t s_conversion(const __uint512_t &block) const {
        __uint512_t result;
        for (const auto &value : block) {
            result.push_back(pi_transformation(value));
        }
        return result;
    }

    __uint512_t p_conversion(const __uint512_t &block) const {
        const auto &tau = constants::tau_transformation::tau;

        std::vector<uint8_t> bytes;
        for (int i = 3; i >= 0; i--) {
            auto value = block[i];
            for (int j = 0; j < 16; j++) {
                bytes.push_back((uint8_t)value);
                value >>= 8;
            }
        }

        std::reverse(bytes.begin(), bytes.end());
        std::vector<uint8_t> replaced;
//        for (int i = 63; i >= 0; i--) {
//            replaced.push_back(bytes[tau[i]]);
//        }
        for (int i : tau) {
            replaced.push_back(bytes[i]);
        }

        __uint512_t result;
        for (int i = 3; i >= 0; i--) {
            __uint128_t value = 0;
            for (int j = 0; j < 16; j++) {
                value <<= 8;
                value |= replaced[j + (3 - i) * 16];
            }
            result.push_back(value);
        }

        return result;
    }

    __uint512_t l_conversion(const __uint512_t &block) const {
        __uint512_t result;
        for (const auto &value : block) {
            auto first_part = (__uint128_t) linear_transformation((uint64_t) (value >> 64));
            auto second_part = (__uint128_t) linear_transformation((uint64_t) value);
            result.push_back((first_part << 64) | second_part);
        }
        return result;
    }

    __uint512_t xor_conversion(const __uint512_t &block1, const __uint512_t &block2) const {
        __uint512_t result;
        for (auto i = 0; i < block1.size(); i++) {
            result.push_back(block1[i] ^ block2[i]);
        }
        return result;
    }

    __uint512_t lps_function(const __uint512_t &block) const {
        return l_conversion(p_conversion(s_conversion(block)));
    }

    __uint512_t e_function(const __uint512_t &h, const __uint512_t &m) const {
        const auto &c_values = constants::iteration_constants::get_iteration_constants();
        auto key = h;
        auto value = xor_conversion(key, m);

//        std::cout << "e_function" << std::endl;
        //utils::print_hex_array(key, "key");
        //utils::print_hex_array(c_values[0], "c_value[0]");
        //utils::print_hex_array(value, "X[k1](m)");

        for (int i = 1; i < 13; i++) {
            //utils::print_hex_array(key, "key on iteration " + std::to_string(i));
            //utils::print_hex_array(xor_conversion(key, c_values[i - 1]), "key^c[i-1] on iteration " + std::to_string(i));
            //utils::print_hex_array(s_conversion(xor_conversion(key, c_values[i - 1])), "s(key^c[i-1]) on iteration " + std::to_string(i));
            //utils::print_hex_array(p_conversion(s_conversion(xor_conversion(key, c_values[i - 1]))), "p(s(key^c[i-1])) on iteration " + std::to_string(i));
            //utils::print_hex_array(l_conversion(p_conversion(s_conversion(xor_conversion(key, c_values[i - 1])))), "l(p(s(key^c[i-1]))) on iteration " + std::to_string(i));
            key = lps_function(xor_conversion(key, c_values[i - 1]));

            //utils::print_hex_array(lps_function(value), "value after iteration " + std::to_string(i));
            value = xor_conversion(key, lps_function(value));
        }
        //utils::print_hex_array(value, "value");
        return value;
    }

    __uint512_t g_function(const __uint512_t &h, const __uint512_t &m, const __uint512_t &N) const {
        //utils::print_hex_array(h, "h");
        //utils::print_hex_array(N, "N");
        //utils::print_hex_array(xor_conversion(h, N), "xor h, N");
        //utils::print_hex_array(s_conversion(xor_conversion(h, N)), "S(H(h, N))");

        const auto &stage1 = xor_conversion(h, N);
        const auto &stage2 = lps_function(stage1);
        //utils::print_hex_array(stage2, "key");

        const auto &stage3 = e_function(stage2, m);
        const auto &stage4 = xor_conversion(stage3, h);
        const auto &stage5 = xor_conversion(stage4, m);
        return stage5;
    }

    __uint512_t addition(const __uint512_t &block1, const __uint512_t &block2) const {
        __uint512_t result = {0, 0, 0, 0};
        for (int i = 3; i >= 0; i--) {
            result[i] = block1[i] + block2[i];
            // проверка на переполнение
            if (result[i] < block1[i] && i > 0) {
                result[i - 1] += 1;
            }
        }
        return result;
    };

    std::vector<__uint128_t> right_shifting(const std::vector<__uint128_t> &message, size_t bits_length) const {
        std::vector<__uint128_t> result;

        /**
         * xxxxxxxxxxx000000000
         * < payload >< shift >
         */
        auto shift_size = 128 - bits_length % 128;
        for (int i = (int)message.size() - 1; i >= 0; i--) {
            __uint128_t cur_part = message[i];
            if (i != (int)message.size() - 1) {
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

    std::vector<__uint512_t> add_padding_and_group(const std::vector<__uint128_t> &message,
                                                   size_t bits_length) const {
        std::vector<__uint128_t> message_ = right_shifting(message, bits_length);

        if (bits_length % 512 != 0) {
            std::reverse(message_.begin(), message_.end());
            if (bits_length % 128 != 0) {
                auto value = message_.back();
                value |= (__int128_t)1 << (bits_length % 128);
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

        std::vector<__uint512_t> result;

        for (auto i = 0; i < total_blocks; i++) {
            result.emplace_back();
            for (int j = 0; j < 4; j++) {
                result.back().push_back(message_[i * 4 + j]);
            }
        }
        return result;
    }



public:
    explicit Stribog_hash(__uint512_t IV) {
        IV_ = IV;
    }

    /**
     * Полагаем, что сообщение придет с выравниванием к правому краю (если число бит не кратно 128,
     * то незаполненным будет 0 элемент массива, а не последний).
     */
    std::vector<__uint128_t> hash(const std::vector<__uint128_t> &message,
                                  size_t bits_length,
                                  size_t hash_bits = 512) {
        auto grouped_message = add_padding_and_group(message, bits_length);
        __uint512_t h = IV_;
        __uint512_t N = {0, 0, 0, 0};
        __uint512_t Sigma = {0, 0, 0, 0};

        std::reverse(grouped_message.begin(), grouped_message.end());

        //utils::print_hex_array(grouped_message);

        for (auto i = 0; i < grouped_message.size() - 1; i++) {
            h = g_function(h, grouped_message[i], N);
            N = addition(N, {0, 0, 0, 512});
            Sigma = addition(Sigma, grouped_message[i]);
        };

        h = g_function(h, grouped_message.back(), N);
        //utils::print_hex_array(h, "h value");
        N = addition(N, {0, 0, 0, bits_length});
        //utils::print_hex_array(N, "N value");
        Sigma = addition(Sigma, grouped_message.back());
        //utils::print_hex_array(Sigma, "Sigma value");
        h = g_function(h, N, {0, 0, 0, 0});
        h = g_function(h, Sigma, {0, 0, 0, 0});

        if (hash_bits == 256) {
            h = {h[0], h[1]};
        }

        return h;
    }

private:

    __uint512_t IV_;
};

void test_utils() {
    const char* message_str = "00112233445566778899AABBCCDDEEFF\0";
    auto value = utils::parse_hex<__uint128_t>(message_str);
    utils::print_hex<__uint128_t>(value);
};

void test_stribog() {

    std::cout << std::endl << "test_stribog" << std::endl << std::endl;

    Stribog_hash stribog({0, 0, 0, 0});
    const char* message_str = "323130393837363534333231303938373635343332313039383736353433323130393837363534333231303938373635343332313039383736353433323130\0";

    auto message_length = strlen(message_str);

    std::vector<__uint128_t> message;
    for (int i = 0; i < 4; i++) {
        message.push_back(utils::parse_hex<__uint128_t>(message_str + 32 * i));
    }

    for (int i = 0; i < 4; i++) {
        utils::print_hex(message[i]);
    }

    std::cout << std::endl << std::endl;

    auto hash = stribog.hash(message, message_length * 4);

    for(const auto& value : hash) {
        utils::print_hex(value);
    }


    const size_t SIZE = 10 * 1000;
    auto time_begin = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++) {
        stribog.hash(hash, 512);
    }
    auto time_end = std::chrono::steady_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count();
    std::cerr << "encode: " << 512 / 8.0 * SIZE / milliseconds / 1024.0 << " MB/s" << std::endl;
}

int main() {

    test_utils();

    test_stribog();

    return 0;
}