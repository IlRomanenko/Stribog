#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "constants.h"

class Stribog_hash {

    inline uint8_t *xor_conversion(uint8_t *block512_1, const uint8_t *block512_2) const {
        for (int i = 0; i < 64; i++) {
            block512_1[i] ^= block512_2[i];
        }
        return block512_1;
    }

    inline uint8_t *lps_function(uint8_t *block512) const {
        static const auto &transform_matrix = constants::linear_transformation::precalc_linear_matrix();

        static uint64_t memory[8];
        uint64_t result = 0;
        for (int i = 0; i < 8; i++) {
            result = 0;
            for (int j = 0; j < 8; j++) {
                result ^= transform_matrix[j][block512[i + j * 8]];
            }
            memory[i] = result;
        }
        memcpy(block512, memory, 64);
        return block512;
    }

    inline uint8_t *e_function(uint8_t *h512, const uint8_t *m512) const {
        static const auto &c_values = constants::iteration_constants::get_iteration_constants();
        static uint8_t memory[64];
        memcpy(memory, h512, 64);
        auto key = memory;
        auto value = xor_conversion(h512, m512);

        for (int i = 1; i < 13; i++) {
            key = lps_function(xor_conversion(key, c_values + (i - 1) * 64));
            value = xor_conversion(lps_function(value), key);
        }
       return h512;
    }

    inline uint8_t *g_function(uint8_t *h512, const uint8_t *m512, const uint8_t *N512) const {
        static uint8_t memory[64];
        memcpy(memory, h512, 64);

        const auto &stage1 = xor_conversion(memory, N512);
        const auto &stage2 = lps_function(stage1);
        const auto &stage3 = e_function(stage2, m512);
        const auto &stage4 = xor_conversion(stage3, h512);
        const auto &stage5 = xor_conversion(stage4, m512);
        memcpy(h512, memory, 64);
        return h512;
    }

    inline uint8_t *addition(uint8_t *block512_1, const uint8_t *block512_2) const {
        for (int i = 63; i >= 0; i--) {
            block512_1[i] += block512_2[i];
            // проверка на переполнение
            if (block512_1[i] < block512_2[i] && i > 0) {
                block512_1[i - 1] += 1;
            }
        }
        return block512_1;
    };


public:
    explicit Stribog_hash(const uint8_t *IV512) {
        IV_ = utils::copy<64>(IV512);
    }

    /**
     * Полагаем, что сообщение придет с выравниванием к правому краю (если число бит не кратно 128,
     * то незаполненным будет 0 элемент массива, а не последний).
     */
    void hash(
            uint8_t * output,
            const uint8_t * message,
            size_t bits_length,
            size_t hash_bits = 512) {
        static auto * h = new uint8_t[64];
        static auto * N = new uint8_t[64];
        static auto * Sigma = new uint8_t[64];
        memcpy(h, IV_, 64);
        memset(N, 0, 64);
        memset(Sigma, 0, 64);

        static const auto& addition_512 = utils::get_block_with_value<uint64_t, 64>(512);

        auto total_blocks = bits_length / 512 + (bits_length % 512 ? 1 : 0);

        for (auto i = 0; i < total_blocks - 1; i++) {
            h = g_function(h, message + 64 * i, N);
            N = addition(N, addition_512);
            Sigma = addition(Sigma, message + 64 * i);
        }

        h = g_function(h, message + 64 * (total_blocks - 1), N);
        N = addition(N, utils::get_block_with_value<uint64_t, 64>(bits_length));
        Sigma = addition(Sigma, message + 64 * (total_blocks - 1));
        h = g_function(h, N, utils::empty_block<64>());
        h = g_function(h, Sigma, utils::empty_block<64>());
        memcpy(output, h, 64);
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
    message.reserve(4);
    for (int i = 0; i < 4; i++) {
        message.push_back(utils::parse_hex<__uint128_t>(message_str + 32 * i));
    }

    for (int i = 0; i < 4; i++) {
        utils::print_hex(message[i]);
    }

    auto memory_message = utils::add_padding(message, message_length * 4);

    utils::print_hex(memory_message, 64);
    std::cout << std::endl << std::endl;

    auto hash = new uint8_t[64];
    stribog.hash(hash, memory_message, message_length * 4);
    utils::print_hex(hash, 64);
    const char *result_hash = "486f64c1917879417fef082b3381a4e211c324f074654c38823a7b76f830ad00fa1fbae42b"
                              "1285c0352f227524bc9ab16254288dd6863dccd5b9f54a1ad0541b\0";
    std::cout << result_hash << std::endl;

    const size_t SIZE = 1 * 1000 * 1000;
    auto time_begin = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++) {
        stribog.hash(hash, hash, 512);
    }
    auto time_end = std::chrono::steady_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count();
    std::cerr << "encode: " << 64.0 * SIZE * 1000 / milliseconds / 1024.0 / 1024.0 << " MB/s" << std::endl;
}

int main() {

    test_utils();
    test_stribog();

    return 0;
}