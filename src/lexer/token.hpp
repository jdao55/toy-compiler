#ifndef __TOKEN_H_
#define __TOKEN_H_
#include <bits/stdint-intn.h>
#include <string>
#include <variant>
enum token_t : uint8_t {
    tok_eof = 0,

    // commands
    tok_def = 1,
    tok_extern = 2,

    // primary
    tok_identifier = 3,
    tok_i32_literal = 4,
    tok_f32_literal = 5,

    // control
    tok_if = 6,
    tok_then = 7,
    tok_else = 8,
    tok_for = 9,
    tok_in = 10,

    // user defined operators
    tok_binary = 11,
    tok_unary = 12,

    // var definition
    tok_var = 13,

    // assignment op
    tok_equal = 14,

    // brakets commas, and semis
    tok_leftbracket = 15,
    tok_rightbracket = 16,
    tok_leftsqr = 17,
    tok_rightsqr = 18,
    tok_comma = 19,
    tok_semi = 20,

    // operators
    tok_binop = 21,

    // types
    tok_i32 = 22,
    tok_f32 = 23,
    tok_u8 = 24,

};

struct token
{
    token_t type;
    std::variant<double, int32_t, std::string> val = std::string("");
    token(token_t t, const std::variant<double, int32_t, std::string> d) : type(t), val(d) {}

    explicit token(token_t t) : type(t), val("") {}


    bool operator==(const token_t tok) const { return type == tok; }
    bool operator!=(const token_t tok) const { return type != tok; }
    template<typename T> constexpr T get() { return std::get<T>(val); }
};

#endif// __TOKEN_H_
