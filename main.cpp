#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "constants.h"

using __uint512_t = std::array<__uint128_t, 4>;

class Stribog_hash {

    inline uint8_t *linear_transformation(uint8_t *block128) const {
        static const auto &transform_matrix = constants::linear_transformation::get_linear_matrix();

        auto value1 = (uint8_t) block128[7];
        auto value2 = (uint8_t) block128[15];

        uint64_t result1 = 0;
        uint64_t result2 = 0;
        for (int i = 0; i < 64; i++) {
            if (i % 8 == 0) {
                value1 = block128[7 - i / 8];
                value2 = block128[15 - i / 8];
            }

            const auto &value = transform_matrix[i];
            if (value1 & 1) {
                result1 ^= value;
            }
            if (value2 & 1) {
                result2 ^= value;
            }
            value1 >>= 1;
            value2 >>= 1;
        }
        memcpy(block128, (uint8_t*)(&result1), 8);
        memcpy(block128 + 8, (uint8_t*)(&result2), 8);
        return block128;
    }

    inline uint8_t *pi_transformation(uint8_t *block128) const {
        static const auto &pi = constants::pi_transformation::pi;
        for (int i = 0; i < 16; i++) {
            block128[i] = pi[block128[i]];
        }
        return block128;
    }

    uint8_t *s_conversion(uint8_t *block512) const {
        for (int i = 0; i < 4; i++) {
            pi_transformation(block512 + 16 * i);
        }
        return block512;
    }

    uint8_t *p_conversion(uint8_t *block512) const {
        static const auto &tau = constants::tau_transformation::tau;
        static uint8_t arr[64];

        memcpy(arr, block512, 64);

        for (int i = 0; i < 64; i++) {
            block512[i] = arr[tau[i]];
        }
        return block512;
    }

    uint8_t *l_conversion(uint8_t *block512) const {
        for (int i = 0; i < 4; i++) {
             linear_transformation(block512 + 16 * i);
        }
        return block512;
    }

    uint8_t *xor_conversion(uint8_t *block512_1, const uint8_t *block512_2) const {
        for (auto i = 0; i < 64; i++) {
            block512_1[i] = block512_1[i] ^ block512_2[i];
        }
        return block512_1;
    }

    uint8_t *lps_function(uint8_t *block512) const {
        return l_conversion(p_conversion(s_conversion(block512)));
    }

    uint8_t *e_function(const uint8_t *h512, const uint8_t *m512) const {
        const auto &c_values = constants::iteration_constants::get_iteration_constants();
        auto key = utils::copy<64>(h512);
        auto value = xor_conversion(utils::copy<64>(key), m512);

//        std::cout << "e_function" << std::endl;
//        utils::print_hex_array(key, "key");
//        utils::print_hex_array(c_values, "c_value[0]");
//        utils::print_hex_array(value, "X[k1](m)");

        for (int i = 1; i < 13; i++) {
//            utils::print_hex_array(key, "key on iteration " + std::to_string(i));
//            utils::print_hex_array(xor_conversion(utils::copy<64>(key), c_values + 64 * (i - 1)), "key^c[i-1] on iteration " + std::to_string(i));
//            utils::print_hex_array(s_conversion(xor_conversion(utils::copy<64>(key), c_values + 64 * (i - 1))), "s(key^c[i-1]) on iteration " + std::to_string(i));
//            utils::print_hex_array(p_conversion(s_conversion(xor_conversion(utils::copy<64>(key), c_values + 64 * (i - 1)))), "p(s(key^c[i-1])) on iteration " + std::to_string(i));
//            utils::print_hex_array(l_conversion(p_conversion(s_conversion(xor_conversion(utils::copy<64>(key), c_values + 64 * (i - 1))))), "l(p(s(key^c[i-1]))) on iteration " + std::to_string(i));
            key = lps_function(xor_conversion(key, c_values + (i - 1) * 64));

//            utils::print_hex_array(lps_function(utils::copy<64>(value)), "value after iteration " + std::to_string(i));
            value = xor_conversion(lps_function(value), key);
        }
        //utils::print_hex_array(value, "value");
        return value;
    }

    uint8_t *g_function(const uint8_t *h512, const uint8_t *m512, const uint8_t *N512) const {
//        utils::print_hex_array(h512, "h");
//        utils::print_hex_array(N512, "N");
//        utils::print_hex_array(xor_conversion(utils::copy<64>(h512), N512), "xor h, N");
//        utils::print_hex_array(s_conversion(xor_conversion(utils::copy<64>(h512), N512)), "S(H(h, N))");

        const auto &stage1 = xor_conversion(utils::copy<64>(h512), N512);
        const auto &stage2 = lps_function(stage1);
//        utils::print_hex_array(stage2, "key");

        const auto &stage3 = e_function(stage2, m512);
        const auto &stage4 = xor_conversion(stage3, h512);
        const auto &stage5 = xor_conversion(stage4, m512);
        return stage5;
    }

    uint8_t *addition(uint8_t *block512_1, const uint8_t *block512_2) const {
        for (int i = 63; i >= 0; i--) {
            block512_1[i] += block512_2[i];
            // проверка на переполнение
            if (block512_1[i] < block512_2[i] && i > 0) {
                block512_1[i - 1] += 1;
            }
        }
//
//        __uint512_t result = {0, 0, 0, 0};
//        for (int i = 3; i >= 0; i--) {
//            result[i] = block1[i] + block2[i];
//            // проверка на переполнение
//            if (result[i] < block1[i] && i > 0) {
//                result[i - 1] += 1;
//            }
//        }
        return block512_1;
    };

    std::vector<__uint128_t> right_shifting(const std::vector<__uint128_t> &message, size_t bits_length) const {
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

    std::vector<uint8_t *> add_padding_and_group(const std::vector<__uint128_t> &message,
                                                   size_t bits_length) const {
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

        std::vector<uint8_t *> result;

        for (auto i = 0; i < total_blocks; i++) {
            result.push_back(new uint8_t[64]);
            for (int j = 0; j < 4; j++) {
                utils::deconvert<__uint128_t>(message_[i * 4 + j], result[result.size() - 1] + j * 16);
            }
        }
        return result;
    }



public:
    explicit Stribog_hash(const uint8_t *IV512) {
        IV_ = utils::copy<64>(IV512);
    }

    /**
     * Полагаем, что сообщение придет с выравниванием к правому краю (если число бит не кратно 128,
     * то незаполненным будет 0 элемент массива, а не последний).
     */
    std::vector<__uint128_t> hash(const std::vector<__uint128_t> &message,
                                  size_t bits_length,
                                  size_t hash_bits = 512) {
        auto grouped_message = add_padding_and_group(message, bits_length);
        auto h = utils::copy<64>(IV_);
        auto N = new uint8_t[64];
        auto Sigma = new uint8_t[64];

        std::reverse(grouped_message.begin(), grouped_message.end());

//        utils::print_hex_array(grouped_message);

        static const auto& addition_512 = utils::get_block_with_value<uint64_t, 64>(512);

        for (auto i = 0; i < grouped_message.size() - 1; i++) {
            h = g_function(h, grouped_message[i], N);
            N = addition(N, addition_512);
            Sigma = addition(Sigma, grouped_message[i]);
        };

        h = g_function(h, grouped_message.back(), N);
//        utils::print_hex_array(h, "h value");
        N = addition(N, utils::get_block_with_value<uint64_t, 64>(bits_length));
        //utils::print_hex_array(N, "N value");
        Sigma = addition(Sigma, grouped_message.back());
        //utils::print_hex_array(Sigma, "Sigma value");
        h = g_function(h, N, utils::empty_block<64>());
        h = g_function(h, Sigma, utils::empty_block<64>());

        if (hash_bits == 256) {
            return {utils::convert<__uint128_t>(h), utils::convert<__uint128_t>(h + 16)};
        }
        return {utils::convert<__uint128_t>(h), utils::convert<__uint128_t>(h + 16),
                utils::convert<__uint128_t>(h + 16 * 2), utils::convert<__uint128_t>(h + 16 * 3)};
    }

private:

    uint8_t *IV_;
};

void test_utils() {
    const char *message_str = "00112233445566778899AABBCCDDEEFF\0";
    auto value = utils::parse_hex<__uint128_t>(message_str);
    utils::print_hex<__uint128_t>(value);
};

void test_stribog() {

    std::cout << std::endl << "test_stribog" << std::endl << std::endl;

    Stribog_hash stribog(utils::empty_block<64>());
    const char *message_str = "323130393837363534333231303938373635343332313039383736353433323130393837363534333231303938373635343332313039383736353433323130\0";

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

    for (const auto &value : hash) {
        utils::print_hex(value);
    }
    const char *result_hash = "486f64c1917879417fef082b3381a4e211c324f074654c38823a7b76f830ad00fa1fbae42b1285c0352f227524bc9ab16254288dd6863dccd5b9f54a1ad0541b\0";
    std::cout << result_hash << std::endl;

    const size_t SIZE = 1 * 1000;
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