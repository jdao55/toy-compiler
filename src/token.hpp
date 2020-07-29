#ifndef __TOKEN_H_
#define __TOKEN_H_
#include <string>
enum tokens_t {
  eof = 0,
  equal,
  eequals,
  notequals,
  less_than,
  greater_than,
  less_than_eq,
  greater_than_eq,
  identifier,
  integer,
  floating_point,
  left_bracket,
  right_bracket,
  left_cbracket,
  right_cbracket,
  plus,
  minus,
  mult,
  fwdslash,
  comma,
  semi
};

struct token {
  tokens_t type;
  std::string text;
  token(tokens_t t, const std::string &s) : type(t), text(s) {}

  explicit token(tokens_t t) : type(t) {}
};

#endif // __TOKEN_H_
