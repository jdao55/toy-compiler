#ifndef AST_HPP
#define AST_HPP

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "../codegen/codegen.hpp"
/// ExprAST - Base class for all expression nodes.
class ExprAST
{
  public:
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen(CodeModule &code_module) = 0;
};


class FnAST
{
  public:
    virtual ~FnAST() = default;

    virtual llvm::Function *codegen(CodeModule &code_module) = 0;
};

// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST
{
    double Val;


  public:
    explicit NumberExprAST(double _val) : Val(_val) {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST
{
    std::string Name;

  public:
    explicit VariableExprAST(const std::string &_name) : Name(_name) {}

    llvm::Value *codegen(CodeModule &code_module) override;
    const std::string &getName() const { return Name; }
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST
{
    char Opcode;
    std::unique_ptr<ExprAST> Operand;

  public:
    UnaryExprAST(char _opcode, std::unique_ptr<ExprAST> _operand) : Opcode(_opcode), Operand(std::move(_operand)) {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

  public:
    BinaryExprAST(char _op, std::unique_ptr<ExprAST> _lhs, std::unique_ptr<ExprAST> _rhs)
        : Op(_op), LHS(std::move(_lhs)), RHS(std::move(_rhs))
    {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

  public:
    CallExprAST(const std::string &_callee, std::vector<std::unique_ptr<ExprAST>> _args)
        : Callee(_callee), Args(std::move(_args))
    {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST
{
    std::unique_ptr<ExprAST> Cond, Then, Else;

  public:
    IfExprAST(std::unique_ptr<ExprAST> _cond, std::unique_ptr<ExprAST> _then, std::unique_ptr<ExprAST> _else)
        : Cond(std::move(_cond)), Then(std::move(_then)), Else(std::move(_else))
    {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST
{
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

  public:
    ForExprAST(const std::string &_varName,
        std::unique_ptr<ExprAST> _start,
        std::unique_ptr<ExprAST> _end,
        std::unique_ptr<ExprAST> _step,
        std::unique_ptr<ExprAST> _body)
        : VarName(_varName), Start(std::move(_start)), End(std::move(_end)), Step(std::move(_step)),
          Body(std::move(_body))
    {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// VarExprAST - Expression class for var/in
class VarExprAST : public ExprAST
{
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::unique_ptr<ExprAST> Body;

  public:
    VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> _varNames, std::unique_ptr<ExprAST> _body)
        : VarNames(std::move(_varNames)), Body(std::move(_body))
    {}

    llvm::Value *codegen(CodeModule &code_module) override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST : public FnAST
{
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;
    uint32_t Precedence;// Precedence if a binary op.

  public:
    PrototypeAST(const std::string &name, std::vector<std::string> args, bool isOperator = false, unsigned prec = 0)
        : Name(name), Args(std::move(args)), IsOperator(isOperator), Precedence(prec)
    {}

    llvm::Function *codegen(CodeModule &code_module);
    const std::string &getName() const { return Name; }

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

    char getOperatorName() const
    {
        assert(isUnaryOp() || isBinaryOp());
        return Name[Name.size() - 1];
    }

    uint32_t getBinaryPrecedence() const { return Precedence; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public FnAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

  public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
        : Proto(std::move(proto)), Body(std::move(body))
    {}

    llvm::Function *codegen(CodeModule &code_module);
};

#endif
