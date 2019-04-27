#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "Stribog.h"
#include "HMAC.h"


void test_utils() {
    const char *message_str = "00112233445566778899AABBCCDDEEFF\0";
    auto value = utils::parse_hex<__uint128_t>(message_str);
    utils::print_hex<__uint128_t>(value);
};

void test_stribog() {

    std::cout << std::endl << "test_stribog" << std::endl << std::endl;

    Stribog stribog(utils::empty_block<64>());
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

    const size_t SIZE = 2 * 1000 * 1000;
    auto time_begin = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++) {
        stribog.hash(hash, hash, 512);
    }
    auto time_end = std::chrono::steady_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count();
    std::cerr << "encode: " << 64.0 * SIZE * 1000 / milliseconds / 1024.0 / 1024.0 << " MB/s" << std::endl;
}

void test_hmac() {

    auto K = utils::parse_hex_string("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    auto T = utils::parse_hex_string("0126bdb87800af214341456563780100");


    utils::print_hex(K, 32);
    utils::print_hex(T, 16);

    HMAC hmac;
    auto res = hmac(K, 32, T, 16);
    utils::print_hex(res, 64);
}

int main() {

    test_utils();
    test_stribog();

//    test_hmac();
    return 0;
}