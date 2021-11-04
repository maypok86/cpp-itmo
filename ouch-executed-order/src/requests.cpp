#include "requests.h"

#include <functional>

std::string decode_string(const std::vector<unsigned char>::const_iterator & begin, const std::size_t count_bites, const std::function<bool(unsigned char)> & test)
{
    std::string s;
    std::copy_if(begin, begin + count_bites, std::back_inserter(s), test);
    return s;
}

std::string decode_token(const std::vector<unsigned char>::const_iterator & begin, const std::size_t count_bites)
{
    return decode_string(begin, count_bites, [](const unsigned char ch) {
        return ch != ' ' && (std::isalnum(ch) != 0);
    });
}

std::string decode_alpha(const std::vector<unsigned char>::const_iterator & begin, const std::size_t count_bites)
{
    return decode_string(begin, count_bites, [](const unsigned char ch) {
        return ch != ' ' && (std::isalpha(ch) != 0);
    });
}

std::uint8_t get_bit(const std::uint8_t mask, const std::uint8_t number)
{
    return mask & (1u << number);
}

bool decode_bit(const std::uint8_t mask, const std::uint8_t number)
{
    return get_bit(mask, number) != 0;
}

void decode_bit_mask(ExecutionDetails & exec_details, const std::uint8_t mask)
{
    exec_details.self_trade = decode_bit(mask, 7);
    exec_details.internalized = decode_bit(mask, 5);
    if (!decode_bit(mask, 4)) {
        if (decode_bit(mask, 3)) {
            exec_details.liquidity_indicator = LiquidityIndicator::Removed;
        }
        else {
            exec_details.liquidity_indicator = LiquidityIndicator::Added;
        }
    }
    else {
        exec_details.liquidity_indicator = LiquidityIndicator::None;
    }
}

unsigned decode_integer(const unsigned char * numbers, const std::size_t count_bytes)
{
    unsigned integer = 0;
    unsigned pow_bytes = 1;
    for (std::size_t i = 0; i < count_bytes; ++i) {
        integer += numbers[count_bytes - i - 1] * pow_bytes;
        pow_bytes *= 256;
    }
    return integer;
}

double decode_price(const unsigned char * numbers, const std::size_t count_bytes)
{
    return static_cast<double>(decode_integer(numbers, count_bytes)) / 10000;
}

ExecutionDetails decode_executed_order(const std::vector<unsigned char> & message)
{
    ExecutionDetails exec_details;
    exec_details.cl_ord_id = decode_token(message.begin() + 9, 14);
    exec_details.filled_volume = decode_integer(&message[23], 4);
    exec_details.price = decode_price(&message[27], 4);
    exec_details.match_number = decode_integer(&message[32], 4);
    exec_details.counterpart = decode_alpha(message.begin() + 36, 4);
    decode_bit_mask(exec_details, static_cast<std::uint8_t>(message[43]));
    return exec_details;
}
