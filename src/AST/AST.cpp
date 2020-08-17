#include "llvm/IR/Verifier.h"
#include "AST.hpp"
#include "../misc/util.hpp"
#include <fmt/format.h>

std::map<std::string, uint32_t> BinopPrecedence{ { "<", 10 }, { "+", 20 }, { "-", 20 }, { "*", 40 } };

llvm::Value *LogErrorV(std::string_view Str) { return util::logError<llvm::Value *>(Str); }

llvm::Function *getFunction(std::string Name, CodeModule &code_module)
{
    // First, see if the function has already been added to the current module.
    if (auto *F = code_module.TheModule->getFunction(Name)) return F;

    // If not, check whether we can codegen the declaration from some existing
    // prototype.
    auto FI = code_module.FunctionProtos.find(Name);
    if (FI != code_module.FunctionProtos.end()) return FI->second->codegen(code_module);

    // If no existing prototype exists, return null.
    return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, llvm::StringRef VarName, CodeModule &code_module)
{
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(llvm::Type::getDoubleTy(code_module.TheContext), nullptr, VarName);
}

llvm::Value *NumberExprAST::codegen(CodeModule &code_module)
{
    return llvm::ConstantFP::get(code_module.TheContext, llvm::APFloat(Val));
}

llvm::Value *VariableExprAST::codegen(CodeModule &code_module)
{
    // Look this variable up in the function.
    llvm::Value *V = code_module.NamedValues[Name];
    if (!V) return LogErrorV(fmt::format("Unknown variable name: {}", Name));

    // Load the value.
    return code_module.Builder.CreateLoad(V, Name.c_str());
}

llvm::Value *UnaryExprAST::codegen(CodeModule &code_module)
{
    llvm::Value *OperandV = Operand->codegen(code_module);
    if (!OperandV) return nullptr;

    llvm::Function *F = getFunction(std::string("unary") + Opcode, code_module);
    if (!F) return LogErrorV("Unknown unary operator");

    return code_module.Builder.CreateCall(F, OperandV, "unop");
}

llvm::Value *BinaryExprAST::codegen(CodeModule &code_module)
{
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == '=')
    {
        // Assignment requires the LHS to be an identifier.
        // This assume we're building without RTTI because LLVM builds that way by
        // default.  If you build LLVM with RTTI this can be changed to a
        // dynamic_cast for automatic error checking.
        VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
        if (!LHSE) return LogErrorV("destination of '=' must be a variable");
        // Codegen the RHS.
        llvm::Value *Val = RHS->codegen(code_module);
        if (!Val) return nullptr;

        // Look up the name.
        llvm::Value *Variable = code_module.NamedValues[LHSE->getName()];
        if (!Variable) return LogErrorV("Unknown variable name");

        code_module.Builder.CreateStore(Val, Variable);
        return Val;
    }

    llvm::Value *L = LHS->codegen(code_module);
    llvm::Value *R = RHS->codegen(code_module);
    if (!L || !R) return nullptr;

    switch (Op)
    {
    case '+':
        return code_module.Builder.CreateFAdd(L, R, "addtmp");
    case '-':
        return code_module.Builder.CreateFSub(L, R, "subtmp");
    case '*':
        return code_module.Builder.CreateFMul(L, R, "multmp");
    case '<':
        L = code_module.Builder.CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return code_module.Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(code_module.TheContext), "booltmp");
    default:
        break;
    }

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    llvm::Function *F = getFunction(std::string("binary") + Op, code_module);
    assert(F && "binary operator not found!");

    llvm::Value *Ops[] = { L, R };
    return code_module.Builder.CreateCall(F, Ops, "binop");
}

llvm::Value *CallExprAST::codegen(CodeModule &code_module)
{
    // Look up the name in the global module table.
    llvm::Function *CalleeF = getFunction(Callee, code_module);
    if (!CalleeF) return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size()) return LogErrorV("Incorrect # arguments passed");

    std::vector<llvm::Value *> ArgsV;
    for (size_t i = 0, e = Args.size(); i != e; ++i)
    {
        ArgsV.push_back(Args[i]->codegen(code_module));
        if (!ArgsV.back()) return nullptr;
    }

    return code_module.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Value *IfExprAST::codegen(CodeModule &code_module)
{
    llvm::Value *CondV = Cond->codegen(code_module);
    if (!CondV) return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    CondV = code_module.Builder.CreateFCmpONE(
        CondV, llvm::ConstantFP::get(code_module.TheContext, llvm::APFloat(0.0)), "ifcond");

    llvm::Function *TheFunction = code_module.Builder.GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(code_module.TheContext, "then", TheFunction);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(code_module.TheContext, "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(code_module.TheContext, "ifcont");

    code_module.Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then value.
    code_module.Builder.SetInsertPoint(ThenBB);

    llvm::Value *ThenV = Then->codegen(code_module);
    if (!ThenV) return nullptr;

    code_module.Builder.CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = code_module.Builder.GetInsertBlock();

    // Emit else block.
    TheFunction->getBasicBlockList().push_back(ElseBB);
    code_module.Builder.SetInsertPoint(ElseBB);

    llvm::Value *ElseV = Else->codegen(code_module);
    if (!ElseV) return nullptr;

    code_module.Builder.CreateBr(MergeBB);
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    ElseBB = code_module.Builder.GetInsertBlock();

    // Emit merge block.
    TheFunction->getBasicBlockList().push_back(MergeBB);
    code_module.Builder.SetInsertPoint(MergeBB);
    llvm::PHINode *PN = code_module.Builder.CreatePHI(llvm::Type::getDoubleTy(code_module.TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:
llvm::Value *ForExprAST::codegen(CodeModule &code_module)
{
    llvm::Function *TheFunction = code_module.Builder.GetInsertBlock()->getParent();

    // Create an alloca for the variable in the entry block.
    llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, code_module);

    // Emit the start code first, without 'variable' in scope.
    llvm::Value *StartVal = Start->codegen(code_module);
    if (!StartVal) return nullptr;

    // Store the value into the alloca.
    code_module.Builder.CreateStore(StartVal, Alloca);

    // Make the new basic block for the loop header, inserting after current
    // block.
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(code_module.TheContext, "loop", TheFunction);

    // Insert an explicit fall through from the current block to the LoopBB.
    code_module.Builder.CreateBr(LoopBB);

    // Start insertion in LoopBB.
    code_module.Builder.SetInsertPoint(LoopBB);

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    llvm::AllocaInst *OldVal = code_module.NamedValues[VarName];
    code_module.NamedValues[VarName] = Alloca;

    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.
    if (!Body->codegen(code_module)) return nullptr;

    // Emit the step value.
    llvm::Value *StepVal = nullptr;
    if (Step)
    {
        StepVal = Step->codegen(code_module);
        if (!StepVal) return nullptr;
    }
    else
    {
        // If not specified, use 1.0.
        StepVal = llvm::ConstantFP::get(code_module.TheContext, llvm::APFloat(1.0));
    }

    // Compute the end condition.
    llvm::Value *EndCond = End->codegen(code_module);
    if (!EndCond) return nullptr;

    // Reload, increment, and restore the alloca.  This handles the case where
    // the body of the loop mutates the variable.
    llvm::Value *CurVar = code_module.Builder.CreateLoad(Alloca, VarName.c_str());
    llvm::Value *NextVar = code_module.Builder.CreateFAdd(CurVar, StepVal, "nextvar");
    code_module.Builder.CreateStore(NextVar, Alloca);

    // Convert condition to a bool by comparing non-equal to 0.0.
    EndCond = code_module.Builder.CreateFCmpONE(
        EndCond, llvm::ConstantFP::get(code_module.TheContext, llvm::APFloat(0.0)), "loopcond");

    // Create the "after loop" block and insert it.
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(code_module.TheContext, "afterloop", TheFunction);

    // Insert the conditional branch into the end of LoopEndBB.
    code_module.Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    code_module.Builder.SetInsertPoint(AfterBB);

    // Restore the unshadowed variable.
    if (OldVal)
        code_module.NamedValues[VarName] = OldVal;
    else
        code_module.NamedValues.erase(VarName);

    // for expr always returns 0.0.
    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(code_module.TheContext));
}

llvm::Value *VarExprAST::codegen(CodeModule &code_module)
{
    std::vector<llvm::AllocaInst *> OldBindings;

    llvm::Function *TheFunction = code_module.Builder.GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    for (size_t i = 0, e = VarNames.size(); i != e; ++i)
    {
        const std::string &VarName = VarNames[i].first;
        ExprAST *Init = VarNames[i].second.get();

        // Emit the initializer before adding the variable to scope, this prevents
        // the initializer from referencing the variable itself, and permits stuff
        // like this:
        //  var a = 1 in
        //    var a = a in ...   # refers to outer 'a'.
        llvm::Value *InitVal;
        if (Init)
        {
            InitVal = Init->codegen(code_module);
            if (!InitVal) return nullptr;
        }
        else
        {// If not specified, use 0.0.
            InitVal = llvm::ConstantFP::get(code_module.TheContext, llvm::APFloat(0.0));
        }

        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, code_module);
        code_module.Builder.CreateStore(InitVal, Alloca);

        // Remember the old variable binding so that we can restore the binding when
        // we unrecurse.
        OldBindings.push_back(code_module.NamedValues[VarName]);

        // Remember this binding.
        code_module.NamedValues[VarName] = Alloca;
    }

    // Codegen the body, now that all vars are in scope.
    llvm::Value *BodyVal = Body->codegen(code_module);
    if (!BodyVal) return nullptr;

    // Pop all our variables from scope.
    for (size_t i = 0, e = VarNames.size(); i != e; ++i) code_module.NamedValues[VarNames[i].first] = OldBindings[i];

    // Return the body computation.
    return BodyVal;
}

llvm::Function *PrototypeAST::codegen(CodeModule &code_module)
{
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> Doubles(Args.size(), llvm::Type::getDoubleTy(code_module.TheContext));
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(code_module.TheContext), Doubles, false);

    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, code_module.TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) Arg.setName(Args[Idx++]);

    return F;
}

llvm::Function *FunctionAST::codegen(CodeModule &code_module)
{
    // Transfer ownership of the prototype to the FunctionProtos map, but keep a
    // reference to it for use below.
    auto &P = *Proto;
    code_module.FunctionProtos[Proto->getName()] = std::move(Proto);
    llvm::Function *TheFunction = getFunction(P.getName(), code_module);
    if (!TheFunction) return nullptr;

    // If this is an operator, install it.
    if (P.isBinaryOp()) BinopPrecedence[P.getName()] = P.getBinaryPrecedence();

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(code_module.TheContext, "entry", TheFunction);
    code_module.Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    code_module.NamedValues.clear();
    for (auto &Arg : TheFunction->args())
    {
        // Create an alloca for this variable.
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), code_module);

        // Store the initial value into the alloca.
        code_module.Builder.CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table.
        code_module.NamedValues[std::string(Arg.getName())] = Alloca;
    }

    if (llvm::Value *RetVal = Body->codegen(code_module))
    {
        // Finish off the function.
        code_module.Builder.CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();

    if (P.isBinaryOp()) BinopPrecedence.erase(P.getName());
    return nullptr;
}
