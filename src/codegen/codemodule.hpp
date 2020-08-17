#ifndef CODE_GEN_HPP
#define CODE_GEN_HPP
//#include "../parser/ToyParser.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include <memory>
//#include "../AST/AST.hpp"


class PrototypeAST;
struct CodeModule
{
    llvm::LLVMContext TheContext;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> TheModule;
    std::map<std::string, llvm::AllocaInst *> NamedValues;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

    CodeModule() : Builder(TheContext), TheModule(std::make_unique<llvm::Module>("Kaleoscope AOT ", TheContext)) {}
};


// // helper type for the visitor #4
// template<class... Ts> struct overloaded : Ts...
// {
//     using Ts::operator()...;
// };
// // explicit deduction guide (not needed as of C++20)
// template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// CodeModule codegen(std::vector<std::variant<ExprAST_ptr, FnAST_ptr>> &top_expressions)
// {
//     CodeModule mod;
//     for (auto &expr : top_expressions)
//     {
//         // 4. another type-matching visitor: a class with 3 overloaded operator()'s
//         std::visit(overloaded{
//                        [&mod](ExprAST &arg) { arg->codegen(mod); },
//                        [&mod](FnAST &arg) { arg->codegen(mod); },
//                    },
//             expr);
//     }
//     return mod;
// }
#endif
