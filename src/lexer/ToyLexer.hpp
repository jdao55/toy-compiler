#ifndef __TOYLEXER_H_
#define __TOYLEXER_H_
#include "token.hpp"
#include <FlexLexer.h>
#include <vector>
class ToyLexer
{
  private:
    std::vector<token> tokenlist;
    std::vector<token>::iterator tok_iter = tokenlist.begin();

  public:
    auto current_token()
    {
        if (tok_iter != tokenlist.end()) { return *tok_iter; }
        else
            return token(tok_eof);
    }
    auto next_token()
    {
        if (tok_iter != tokenlist.end())
        {
            tok_iter++;
            return *tok_iter;
        }
        else
            return token(tok_eof);
    }

    void scan_tokens(std::istream &is = std::cin)

    {
        tokenlist.clear();
        yyFlexLexer lexer;
        lexer.switch_streams(is, std::cout);
        int t;
        while ((t = lexer.yylex()) != 0)
        {
            std::variant<double, int32_t, std::string> val =
                [&t, &lexer]() -> std::variant<double, int32_t, std::string> {
                switch (static_cast<token_t>(t))
                {
                case (token_t::tok_i32_literal):
                    return std::stoi(lexer.YYText());
                case (token_t::tok_f32_literal):
                    return std::stod(lexer.YYText());
                default:
                    return lexer.YYText();
                }
            }();

            // dval = std::stod(lexer.YYText());
            tokenlist.emplace_back(static_cast<token_t>(t), val);
        }
        tok_iter = tokenlist.begin();
    }

    auto begin() { return tokenlist.begin(); }
    auto end() { return tokenlist.end(); }
    auto begin() const { return tokenlist.cbegin(); }
    auto end() const { return tokenlist.cend(); }
};

#endif// __TOYLEXER_H_
