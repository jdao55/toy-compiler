#ifndef __TOYLEXER_H_
#define __TOYLEXER_H_
#include "token.hpp"
#include <FlexLexer.h>
#include <vector>
class ToyLexer {
private:
  std::vector<token> tokenlist;
  std::vector<token>::iterator tok_iter = tokenlist.begin();

public:
  auto current_token() {
    if (tok_iter != tokenlist.end())
      return *tok_iter;
    else
      return token(eof, "eof");
  }
  void next_token() {
    if (tok_iter != tokenlist.end())
      tok_iter++;
  }
  void parse() {
    tokenlist.clear();
    yyFlexLexer lexer;
    int t;
    while ((t = lexer.yylex()) != 0) {
      tokenlist.emplace_back(static_cast<tokens_t>(t), lexer.YYText());
    }
  }
  void parse(std::istream is) {
    tokenlist.clear();
    yyFlexLexer lexer;
    lexer.switch_streams(is, std::cout);
    int t;
    while ((t = lexer.yylex()) != 0) {
      tokenlist.emplace_back(static_cast<tokens_t>(t), lexer.YYText());
    }
  }
  auto begin() { return tokenlist.begin(); }
  auto end() { return tokenlist.end(); }
  auto begin() const { return tokenlist.cbegin(); }
  auto end() const { return tokenlist.cend(); }
};

#endif // __TOYLEXER_H_
