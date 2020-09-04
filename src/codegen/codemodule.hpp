#ifndef CODE_GEN_HPP
#define CODE_GEN_HPP
//#include "../parser/ToyParser.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <memory>
//#include "../AST/AST.hpp"


class PrototypeAST;

struct OptimzerManager
{
    OptimzerManager(llvm::Module *mod, uint8_t optlvl)
        : fn_pass_manager(std::make_unique<llvm::legacy::FunctionPassManager>(mod)),
          mod_pass_manager(std::make_unique<llvm::legacy::PassManager>())
    {
        llvm::PassManagerBuilder pass_builder;
        pass_builder.OptLevel = optlvl <= 3 ? optlvl : 1;
        pass_builder.SizeLevel = 0;
        pass_builder.populateFunctionPassManager(*fn_pass_manager);
        pass_builder.populateModulePassManager(*mod_pass_manager);
    }
    std::unique_ptr<llvm::legacy::FunctionPassManager> fn_pass_manager;
    std::unique_ptr<llvm::legacy::PassManager> mod_pass_manager;
    inline void optimize(llvm::Function *function) { fn_pass_manager->run(*function); }
    inline void optimize(llvm::Module *module) { mod_pass_manager->run(*module); }
};
struct CodeModule
{
    llvm::LLVMContext TheContext;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> TheModule;
    std::unique_ptr<OptimzerManager> opt_mananger;
    std::map<std::string, llvm::AllocaInst *> NamedValues;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

    CodeModule(uint8_t opt_level = 1)
        : Builder(TheContext), TheModule(std::make_unique<llvm::Module>("Kaleoscope AOT ", TheContext)),
          opt_mananger(std::make_unique<OptimzerManager>(TheModule.get(), opt_level))
    {}
};


#endif
