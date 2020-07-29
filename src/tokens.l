%option noyywrap

%{
#include <string>
#include "token.hpp"

%}

%%

[ \t\n]                 ;
[a-zA-Z_][a-zA-Z0-9_]*  return identifier;
[0-9]+.[0-9]*           return floating_point;
[0-9]+                  return integer ;
"="                     return equal ;
"=="                    return eequals ;
"!="                    return notequals;
"<"                     return less_than;
"<="                    return less_than_eq;
">"                     return greater_than;
">="                    return greater_than_eq;
"("                     return left_bracket ;
")"                     return right_bracket;
"{"                     return left_cbracket;
"}"                     return right_cbracket;
"."                     return identifier;
","                     return comma;
"+"                     return plus;
"-"                     return minus;
"*"                     return mult;
"/"                     return fwdslash;
";"                     return semi;
<<EOF>>                 printf("error on tok%s\n", YYText());return tokens_t::eof;                       ;
.                       yyterminate();

%%
