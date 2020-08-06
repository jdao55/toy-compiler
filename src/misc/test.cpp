#include "ToyLexer.hpp"
#include <iostream>
int main()
{
    ToyLexer lexer;
    lexer.scan_tokens();
    std::string cont;
    ;
    while (lexer.current_token() != tok_eof)
    {
        std::cout << "tok: " << static_cast<int>(lexer.current_token().type) << " str: " << lexer.current_token().text
                  << "\n";
        lexer.next_token();
    }
    return 0;
}
