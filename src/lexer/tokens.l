%option noyywrap

%{
#include <string>
#include "token.hpp"

%}
%x COMMENT
%%
"binary"                return tok_binary;
"unary"                 return tok_unary;
"for"                   return tok_for;
"in"                    return tok_in;
"extern"                return tok_extern;
"if"                    return tok_if;
"then"                  return tok_then;
"else"                  return tok_else;
"="                     return tok_equal ;
"=="                    return tok_binop ;
"!="                    return tok_binop;
[<>*+\-/]               return tok_binop;;
"<="                    return tok_binop;
">="                    return tok_binop;
"("                     return tok_leftbracket;
")"                     return tok_rightbracket;
","                     return tok_comma;
";"                     return tok_semi;
[a-zA-Z_][a-zA-Z0-9_]*  return tok_identifier;
[0-9]+(\\.[0-9]*)?      return tok_number;
"/*"                    BEGIN(COMMENT);
\\#[^\n]*\n                   ;
[ \t\n;]                 ;
<<EOF>>                 return tok_eof;
.                       yyterminate();

<COMMENT>
{
    "*/"      BEGIN(INITIAL);
    [^*\n]+   ;
    "*"       ;
    \n        ;
}
%%
