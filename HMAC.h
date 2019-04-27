//
// Created by romanenko-ii on 12/18/18.
//
#pragma once

#include "utils.h"
#include "Stribog.h"

class HMAC {

    inline uint8_t *xor_conversion(uint8_t *block512_1, const uint8_t *block512_2) const {
        for (int i = 0; i < 64; i++) {
            block512_1[i] ^= block512_2[i];
        }
        return block512_1;
    }

public:

    HMAC() {
        stribog = new Stribog(utils::empty_block<64>());
    }

    uint8_t *padded_message(const uint8_t* data, size_t bytes_size) {
        size_t total_bytes = bytes_size;
        if (bytes_size % 16 != 0) {
            total_bytes += 16 - bytes_size % 16;
        }
        auto * T = new uint8_t[total_bytes];
        memset(T, 0, total_bytes);
        memcpy(T, data, bytes_size);

        std::vector<__uint128_t> v;
        size_t total_blocks = total_bytes / 16;
        for (size_t i = 0; i < total_blocks; i++) {
            v.push_back(utils::convert<__uint128_t>(T + 16 * i));
        }

        return utils::add_padding(v, bytes_size * 8);
    }

    uint8_t *operator()(const uint8_t* K, size_t k_size, const uint8_t *T, size_t length) {
        memset(ipad, 0x35, 64);
        memset(opad, 0x5C, 64);

        auto K_padded = new uint8_t[64];
        memset(K_padded, 0, 64);
        memcpy(K_padded, K, k_size);

        xor_conversion(ipad, K_padded);
        xor_conversion(opad, K_padded);

        delete[] K_padded;

        auto *output = new uint8_t[length + 64];
        auto *output2 = new uint8_t[64 * 2];
        memcpy(output, ipad, 64);
        memcpy(output + 64, T, length);

        std::vector<__uint128_t> v;


        auto memory_message = padded_message(output, length);

        stribog->hash(output2 + 64, memory_message, (length + 64) * 8);
        memcpy(output2, opad, 64);
        stribog->hash(output2, output2, (length + 64) * 8);
        delete[] output;
        return output2;
    }

    ~HMAC() {
        delete stribog;
    }

private:
    Stribog *stribog;
    uint8_t ipad[64];
    uint8_t opad[64];
};