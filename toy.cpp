#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace token{
enum Token {
  eof = -1,

  // commnads
  def = -2,
  tok_extern = -3,

  // pimary expressions
  identifier = -4,
  number = -5,
};
}

static std::string identifier_str;
static double num_val;


static int gettok()
{
    static int last_char = ' ';

    while (isspace(last_char))
    {
        last_char = getchar();
    }

    if (isalpha(last_char))
    {
        identifier_str = last_char;
        while (isalnum((last_char = getchar())))
            identifier_str += last_char;

        if (identifier_str == "def")
        {
            return token::def;
        }
        if (identifier_str == "extern")
            return token::tok_extern;
        return token::identifier;
    }
    if (isdigit(last_char) || last_char == '.')
    {
        std::string num_str = "";
        do
        {
            num_str += last_char;
            last_char = getchar();

        } while (isdigit(last_char || last_char == '.'));
        return token::number;
    }
    if (last_char == '#')
    {
        // Comment until end of line.
        do
            last_char = getchar();
        while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF)
            return gettok();
    }

    if (last_char == EOF)
        return token::eof;
    int current_char = last_char;
    last_char = getchar();
    return current_char;
}
