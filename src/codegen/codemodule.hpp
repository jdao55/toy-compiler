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

    CodeModule()
        : Builder(TheContext),
          TheModule(std::make_unique<llvm::Module>("Kaleoscope AOT ", TheContext))
    {}
};

#endif
