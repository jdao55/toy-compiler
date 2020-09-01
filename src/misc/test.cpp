#include "../lexer/ToyLexer.hpp"
#include "../codegen/codegen.hpp"
#include "../parser/ToyParser.hpp"
#include "../argparser/argparser.hpp"
#include <iostream>
#include <fstream>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/Module.h"
int main(int argc, char **argv)
{
    auto args = std::get<Arguments>(get_args(argc, argv));
    ToyParser parser;
    std::ifstream src_stream(args.srcfilename);
    auto expr_vec = parser.MainLoop(src_stream);
    src_stream.close();
    auto mod = codegen(expr_vec);
    // run optimzer on module
    mod->opt_mananger->optimize(mod->TheModule.get());

#ifndef NDEBUG// print llvmIR in debug
    mod->TheModule->print(llvm::errs(), nullptr);
#endif
    // Initialize the target registry etc.
    llvm ::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    mod->TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target)
    {
        llvm::errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    mod->TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(args.outfilename, EC, llvm::sys::fs::OF_None);

    if (EC)
    {
        llvm::errs() << "Could not open file: " << EC.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_ObjectFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
    {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*(mod->TheModule));
    dest.flush();

    llvm::outs() << "Wrote " << args.outfilename << "\n";

    return 0;
}
