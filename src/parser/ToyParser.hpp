#ifndef TOY_PARSER_HPP

#define TOY_PARSER_HPP
#include <fmt/format.h>
#include <map>
#include <variant>
#include "../lexer/ToyLexer.hpp"
#include "../codegen/codemodule.hpp"
#include "../AST/AST.hpp"

class ToyParser
{

  private:
    ToyLexer lexer;

    using ExprAST_ptr = std::unique_ptr<ExprAST>;
    using FnAST_ptr = std::unique_ptr<FnAST>;

  public:
    /// GetTokPrecedence - Get the precedence of the pending binary operator token.

    /// GetTokPrecedence - Get the precedence of the pending binary operator token.
    int GetTokPrecedence()
    {
        // Make sure it's a declared binop.
        auto tok_prec = BinopPrecedence.find(lexer.current_token().get<std::string>());
        if (tok_prec != BinopPrecedence.end()) { return tok_prec->second; }
        return -1;
    }

    /// LogError* - These are little helper functions for error handling.
    std::unique_ptr<ExprAST> LogError(const std::string_view Str)
    {
        fmt::print(stderr, "Error: {}\n", Str);
        return nullptr;
    }

    std::unique_ptr<PrototypeAST> LogErrorP(const std::string_view Str)
    {
        LogError(Str);
        return nullptr;
    }

    /// numberexpr ::= number
    std::unique_ptr<ExprAST> ParseNumberExpr()
    {
        auto Result = std::make_unique<NumberExprAST>(lexer.current_token().get<double>());
        lexer.next_token();// consume the number
        return Result;
    }

    /// parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAST> ParseParenExpr()
    {
        lexer.next_token();// eat (.
        auto V = ParseExpression();
        if (!V) return nullptr;

        if (lexer.current_token() != tok_rightbracket) return LogError("expected ')'");
        lexer.next_token();// eat ).
        return V;
    }

    /// identifierexpr
    ///   ::= identifier
    ///   ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAST> ParseIdentifierExpr()
    {
        std::string IdName = lexer.current_token().get<std::string>();

        lexer.next_token();// eat identifier.

        if (lexer.current_token() != tok_leftbracket)// Simple variable ref.
            return std::make_unique<VariableExprAST>(IdName);

        // Call.
        lexer.next_token();// eat (
        std::vector<std::unique_ptr<ExprAST>> Args;
        if (lexer.current_token() != tok_rightbracket)
        {
            while (true)
            {
                if (auto Arg = ParseExpression())
                    Args.push_back(std::move(Arg));
                else
                    return nullptr;

                if (lexer.current_token() == tok_rightbracket) break;

                if (lexer.current_token() != tok_comma) return LogError("Expected ')' or ',' in argument list");
                lexer.next_token();
            }
        }

        // Eat the ')'.
        lexer.next_token();

        return std::make_unique<CallExprAST>(IdName, std::move(Args));
    }

    /// ifexpr ::= 'if' expression 'then' expression 'else' expression
    std::unique_ptr<ExprAST> ParseIfExpr()
    {
        lexer.next_token();// eat the if.

        // condition.
        auto Cond = ParseExpression();
        if (!Cond) return nullptr;

        if (lexer.current_token() != tok_then)
            return LogError(fmt::format("expected then got: {} ", lexer.current_token().get<std::string>()));
        lexer.next_token();// eat the then

        auto Then = ParseExpression();
        if (!Then) return nullptr;

        if (lexer.current_token() != tok_else) return LogError("expected else");

        lexer.next_token();

        auto Else = ParseExpression();
        if (!Else) return nullptr;

        return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
    }

    /// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
    std::unique_ptr<ExprAST> ParseForExpr()
    {
        lexer.next_token();// eat the for.

        if (lexer.current_token() != tok_identifier) return LogError("expected identifier after for");

        std::string IdName = lexer.current_token().get<std::string>();
        lexer.next_token();// eat identifier.

        if (lexer.current_token() != tok_equal) return LogError("expected '=' after for");
        lexer.next_token();// eat '='.

        auto Start = ParseExpression();
        if (!Start) return nullptr;
        if (lexer.current_token() != tok_comma) return LogError("expected ',' after for start value");
        lexer.next_token();

        auto End = ParseExpression();
        if (!End) return nullptr;

        // The step value is optional.
        std::unique_ptr<ExprAST> Step;
        if (lexer.current_token() == tok_comma)
        {
            lexer.next_token();
            Step = ParseExpression();
            if (!Step) return nullptr;
        }

        if (lexer.current_token() != tok_in) return LogError("expected 'in' after for");
        lexer.next_token();// eat 'in'.

        auto Body = ParseExpression();
        if (!Body) return nullptr;

        return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), std::move(Step), std::move(Body));
    }

    /// varexpr ::= 'var' identifier ('=' expression)?
    //                    (',' identifier ('=' expression)?)* 'in' expression
    std::unique_ptr<ExprAST> ParseVarExpr()
    {
        lexer.next_token();// eat the var.

        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

        // At least one variable name is required.
        if (lexer.current_token() != tok_identifier) return LogError("expected identifier after var");

        while (true)
        {
            std::string Name = lexer.current_token().get<std::string>();
            lexer.next_token();// eat identifier.

            // Read the optional initializer.
            std::unique_ptr<ExprAST> Init = nullptr;
            if (lexer.current_token() == tok_equal)
            {
                lexer.next_token();// eat the '='.

                Init = ParseExpression();
                if (!Init) return nullptr;
            }

            VarNames.push_back(std::make_pair(Name, std::move(Init)));

            // End of var list, exit loop.
            if (lexer.current_token() != tok_equal) break;
            lexer.next_token();// eat the ','.

            if (lexer.current_token() != tok_identifier) return LogError("expected identifier list after var");
        }

        // At this point, we have to have 'in'.
        if (lexer.current_token() != tok_in) return LogError("expected 'in' keyword after 'var'");
        lexer.next_token();// eat 'in'.

        auto Body = ParseExpression();
        if (!Body) return nullptr;

        return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
    }

    /// primary
    ///   ::= identifierexpr
    ///   ::= numberexpr
    ///   ::= parenexpr
    ///   ::= ifexpr
    ///   ::= forexpr
    ///   ::= varexpr
    std::unique_ptr<ExprAST> ParsePrimary()
    {
        auto tok = lexer.current_token().type;
        switch (tok)
        {
        default:
            return LogError(fmt::format("unknown token :{} when expecting an expression", static_cast<int>(tok)));
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_i32_literal:
        case tok_f32_literal:
            return ParseNumberExpr();
        case tok_leftbracket:
            return ParseParenExpr();
        case tok_if:
            return ParseIfExpr();
        case tok_for:
            return ParseForExpr();
        case tok_var:
            return ParseVarExpr();
        }
    }

    /// unary
    ///   ::= primary
    ///   ::= '!' unary
    std::unique_ptr<ExprAST> ParseUnary()
    {
        // If the current token is not an operator, it must be a primary expr.
        if (lexer.current_token().type != tok_binop || lexer.current_token() == tok_leftbracket
            || lexer.current_token() == tok_comma)
            return ParsePrimary();

        // If this is a unary operator, read it.
        int Opc = lexer.current_token().get<std::string>()[0];
        lexer.next_token();
        if (auto Operand = ParseUnary()) return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
        return nullptr;
    }

    /// binoprhs
    ///   ::= ('+' unary)*
    std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
    {
        // If this is a binop, find its precedence.
        while (true)
        {
            int TokPrec = GetTokPrecedence();
            // If this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done.
            if (TokPrec < ExprPrec) return LHS;

            // Okay, we know this is a binop.
            int BinOp = lexer.current_token().get<std::string>()[0];

            lexer.next_token();// eat binop

            // Parse the unary expression after the binary operator.
            auto RHS = ParseUnary();
            if (!RHS) return nullptr;

            // If BinOp binds less tightly with RHS than the operator after RHS, let
            // the pending operator take RHS as its LHS.
            int NextPrec = GetTokPrecedence();
            if (TokPrec < NextPrec)
            {
                RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
                if (!RHS) return nullptr;
            }

            // Merge LHS/RHS.
            LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
        }
    }

    /// expression
    ///   ::= unary binoprhs
    ///
    std::unique_ptr<ExprAST> ParseExpression()
    {
        auto LHS = ParseUnary();
        if (!LHS) return nullptr;

        return ParseBinOpRHS(0, std::move(LHS));
    }

    /// prototype
    ///   ::= id '(' id* ')'
    ///   ::= binary LETTER number? (id, id)
    ///   ::= unary LETTER (id)
    std::unique_ptr<PrototypeAST> ParsePrototype()
    {
        std::string FnName;

        unsigned Kind = 0;// 0 = identifier, 1 = unary, 2 = binary.
        unsigned BinaryPrecedence = 30;

        switch (lexer.current_token().type)
        {
        default:
            return LogErrorP("Expected function name in prototype");
        case tok_identifier:
            FnName = lexer.current_token().get<std::string>();
            Kind = 0;
            lexer.next_token();
            break;
        case tok_unary:
            lexer.next_token();
            if (lexer.current_token() != token_t::tok_binop) return LogErrorP("Expected unary operator");
            FnName = "unary";
            FnName += lexer.current_token().get<std::string>();
            Kind = 1;
            lexer.next_token();
            break;
        case tok_binary:
            lexer.next_token();
            if (lexer.current_token() != token_t::tok_binop) return LogErrorP("Expected binary operator");
            FnName = "binary";
            FnName += lexer.current_token().get<std::string>();
            Kind = 2;
            lexer.next_token();

            // Read the precedence if present.
            if (lexer.current_token() == tok_i32_literal || lexer.current_token() == tok_f32_literal)
            {
                auto NumVal = lexer.current_token().get<double>();
                if (NumVal < 1 || NumVal > 100) return LogErrorP("Invalid precedence: must be 1..100");
                BinaryPrecedence = static_cast<unsigned>(NumVal);
                lexer.next_token();
            }
            break;
        }

        if (lexer.current_token() != tok_leftbracket) return LogErrorP("Expected '(' in prototype");

        std::vector<std::string> ArgNames;
        while (lexer.next_token() == tok_identifier) ArgNames.push_back(lexer.current_token().get<std::string>());
        if (lexer.current_token() != tok_rightbracket)
            return LogErrorP(
                fmt::format("Expected ')' in prototype got: {}", lexer.current_token().get<std::string>()));

        // success.
        lexer.next_token();// eat ')'.

        // Verify right number of names for operator.
        if (Kind && ArgNames.size() != Kind) return LogErrorP("Invalid number of operands for operator");

        return std::make_unique<PrototypeAST>(FnName, ArgNames, Kind != 0, BinaryPrecedence);
    }

    /// definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAST> ParseDefinition()
    {
        lexer.next_token();// eat def.
        auto Proto = ParsePrototype();
        if (!Proto) return nullptr;

        if (auto E = ParseExpression()) return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
        return nullptr;
    }

    /// toplevelexpr ::= expression
    std::unique_ptr<FunctionAST> ParseTopLevelExpr()
    {
        if (auto E = ParseExpression())
        {
            // Make an anonymous proto.
            auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
            return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
        }
        return nullptr;
    }

    /// external ::= 'extern' prototype
    std::unique_ptr<PrototypeAST> ParseExtern()
    {
        lexer.next_token();// eat extern.
        return ParsePrototype();
    }
    /*********************
    ** toplevel parsing
    **********************/
    FnAST_ptr HandleDefinition()
    {
        if (auto FunAST = ParseDefinition())
        {
            return FunAST;
            //     if (auto *FnIR = FnAST->codegen())
            //     {
            //         fprintf(stderr, "Read function definition:");
            //         FnIR->print(errs());
            //         fprintf(stderr, "\n");
            //     }
        }
        else
        {
            // Skip token for error recovery.
            lexer.next_token();
            return FunAST;
        }
    }

    FnAST_ptr HandleExtern()
    {
        if (auto ProtoAST = ParseExtern())
        {
            return ProtoAST;
            /*
             * if (auto *FnIR = ProtoAST->codegen())
            {
                fprintf(stderr, "Read extern: ");
                FnIR->print(errs());
                fprintf(stderr, "\n");
                FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
            }
            */
        }
        // Skip token for error recovery.
        lexer.next_token();
        return nullptr;
    }

    FnAST_ptr HandleTopLevelExpression()
    {
        // Evaluate a top-level expression into an anonymous function.
        if (auto FunAST = ParseTopLevelExpr()) { return FunAST; }
        else
        {
            // Skip token for error recovery.
            lexer.next_token();
            return FunAST;
        }
    }

    auto MainLoop(std::istream &is)
    {
        lexer.scan_tokens(is);
        std::vector<std::variant<ExprAST_ptr, FnAST_ptr>> top_expressions;
        while (lexer.current_token() != tok_eof)
        {
            switch (lexer.current_token().type)
            {
            case tok_eof:
                break;
            case tok_semi:// ignore top-level semicolons.
                lexer.next_token();
                break;
            case tok_def:
                top_expressions.push_back(HandleDefinition());
                break;
            case tok_extern:
                top_expressions.push_back(HandleExtern());
                break;
            default:
                top_expressions.push_back(HandleTopLevelExpression());
                break;
            }
        }
        return top_expressions;
    }
};

#endif
