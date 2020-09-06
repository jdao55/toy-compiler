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
"f32"                   return tok_f32;
"i32"                   return tok_i32;
"u8"                    return tok_u8;
"in"                    return tok_in;
"extern"                return tok_extern;
"if"                    return tok_if;
"then"                  return tok_then;
"else"                  return tok_else;
"def"                   return tok_def;
"var"                   return tok_var;
"="                     return tok_equal ;
"=="                    return tok_binop ;
"!="                    return tok_binop;
[!<>*+\-/]              return tok_binop;;
"<="                    return tok_binop;
">="                    return tok_binop;
"("                     return tok_leftbracket;
")"                     return tok_rightbracket;
"["                     return tok_leftsqr;
"]"                     return tok_rightsqr;
","                     return tok_comma;
";"                     return tok_semi;
[a-zA-Z_][a-zA-Z0-9_]*  return tok_identifier;
[1-9][0-9]*             return tok_i32_literal;
[0-9]+(\\.[0-9]*)?      return tok_f32_literal;
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
