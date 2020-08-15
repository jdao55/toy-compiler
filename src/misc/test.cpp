#include "../lexer/ToyLexer.hpp"
#include "../codegen/codegen.hpp"
#include "../parser/ToyParser.hpp"
#include <iostream>
int main()
{
    ToyParser parser;
    parser.MainLoop(std::cin);
}
