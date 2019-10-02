#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <string_view>
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

//
class ExprAST
{
  public:
    virtual ~ExprAST() {}
};

class NumberExprAST :public ExprAST
{
    double val;
  public:
    NumberExprAST(double _val) : val(_val){}
};

class VariableExprAST :public ExprAST
{
    std::string name;
  public:
    VariableExprAST(std::string &_val) : name(_val){}
    VariableExprAST(std::string &&_val) : name(_val){}
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op,
                std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op),
      LHS(std::move(LHS)),
      RHS(std::move(RHS)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> &Args)
    : Name(name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

static int CurTok;
static int getNextToken() {
  return CurTok = gettok();
}


std::unique_ptr<ExprAST> LogError(const char * Str)
{
  fprintf(stderr, "LogError: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
  LogError(Str);
  return nullptr;
}


static std::unique_ptr<ExprAST> ParseNumberExpr()
{
  auto Result = std::make_unique<NumberExprAST>(num_val);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}
