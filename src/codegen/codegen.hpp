#ifndef __CODEGEN_H_
#define __CODEGEN_H_

#include <memory>
#include <variant>
#include <fmt/format.h>
#include "codemodule.hpp"
#include "../AST/AST.hpp"

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;


using ExprAST_ptr = std::unique_ptr<ExprAST>;
using FnAST_ptr = std::unique_ptr<FnAST>;
inline std::unique_ptr<CodeModule> codegen(
    std::vector<std::variant<ExprAST_ptr, FnAST_ptr>> &top_expressions)
{
    auto mod = std::make_unique<CodeModule>();
    for (auto &expr : top_expressions)
    {

        std::visit(overloaded{
                       [&mod](ExprAST_ptr &arg) { arg->codegen(*mod); },
                       [&mod](FnAST_ptr &arg) { arg->codegen(*mod); },
                   },
            expr);
    }
    return mod;
}
inline void optimize(llvm::Function *function);
inline void optimize(llvm::Module *module);


#endif
// __CODEGEN_H_
