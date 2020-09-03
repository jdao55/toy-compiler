#ifndef __TOKEN_H_
#define __TOKEN_H_
#include <string>
#include <optional>
enum token_t {
    tok_eof = 0,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,

    // control
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_for = -9,
    tok_in = -10,

    // user defined operators
    tok_binary = -11,
    tok_unary = -12,

    // var definition
    tok_var = -13,

    // assignment op
    tok_equal = -14,

    // brakets commas, and semis
    tok_leftbracket = -15,
    tok_rightbracket = -16,
    tok_leftsqr = -23,
    tok_rightsqr = -24,
    tok_comma = -17,
    tok_semi = -18,

    // operators
    tok_binop = -19,

    // types
    tok_u32 = -20,
    tok_f32 = -21,
    tok_u8 = -22,

};

struct token
{
    token_t type;
    std::string text;
    std::optional<double> num_val = std::nullopt;
    token(token_t t, const std::string &s, std::optional<double> d) : type(t), text(s), num_val(d) {}

    explicit token(token_t t) : type(t) {}


    bool operator==(const token_t tok) const { return type == tok; }
    bool operator!=(const token_t tok) const { return type != tok; }
};

#endif// __TOKEN_H_
