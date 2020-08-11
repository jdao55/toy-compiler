#ifndef TOY_PARSER_HPP

#define TOY_PARSER_HPP
#include <fmt/format.h>
#include <map>
#include "../lexer/ToyLexer.hpp"
#include "../AST/AST.hpp"
class ToyParser
{
  private:
    ToyLexer lexer;
    /// BinopPrecedence - This holds the precedence for each binary operator that is
    /// defined.
    static std::map<std::string, int> BinopPrecedence{ { "<", 10 }, { "+", 20 }, { "-", 20 }, { "*", 40 } };
    using ExprAST_ptr = std::unique_ptr<ExprAST>;
    using ProtoAST_ptr = std::unique_ptr<PrototypeAST>;
    using FunctionAST_ptr = std::unique_ptr<FunctionAST>;
    std::vector<std::variant<ExptrAST_ptr, ProtoAST_ptr, FunctionAST_ptr>> > top_expressions;

  public:
    /// GetTokPrecedence - Get the precedence of the pending binary operator token.
    int GetTokPrecedence()
    {
        // Make sure it's a declared binop.
        auto tok_prec = BinopPrecedence.find(lexer.current_token());
        if (tok_prec != BinopPrecedence.end()) { return tok_prec->second; }
        return -1;
    }

    /// LogError* - These are little helper functions for error handling.
    std::unique_ptr<ExprAST> LogError(const std::string_view Str)
    {
        fmt::print(stderr, "Error: {}\n", Str);
        return nullptr;
    }

    std::unique_ptr<PrototypeAST> LogErrorP(const std::string_vew Str)
    {
        LogError(Str);
        return nullptr;
    }

    /// numberexpr ::= number
    std::unique_ptr<ExprAST> ParseNumberExpr()
    {
        auto Result = std::make_unique<NumberExprAST>(NumVal);
        getNextToken();// consume the number
        return std::move(Result);
    }

    /// parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAST> ParseParenExpr()
    {
        getNextToken();// eat (.
        auto V = ParseExpression();
        if (!V) return nullptr;

        if (lexer.current_token() != tok_rightbracket) return LogError("expected ')'");
        getNextToken();// eat ).
        return V;
    }

    /// identifierexpr
    ///   ::= identifier
    ///   ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAST> ParseIdentifierExpr()
    {
        std::string IdName = IdentifierStr;

        getNextToken();// eat identifier.

        if (lexer.current_token() != tok_leftbracket)// Simple variable ref.
            return std::make_unique<VariableExprAST>(IdName);

        // Call.
        getNextToken();// eat (
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
                getNextToken();
            }
        }

        // Eat the ')'.
        getNextToken();

        return std::make_unique<CallExprAST>(IdName, std::move(Args));
    }

    /// ifexpr ::= 'if' expression 'then' expression 'else' expression
    std::unique_ptr<ExprAST> ParseIfExpr()
    {
        getNextToken();// eat the if.

        // condition.
        auto Cond = ParseExpression();
        if (!Cond) return nullptr;

        if (lexer.current_token() != tok_then) return LogError("expected then");
        getNextToken();// eat the then

        auto Then = ParseExpression();
        if (!Then) return nullptr;

        if (lexer.current_token() != tok_else) return LogError("expected else");

        getNextToken();

        auto Else = ParseExpression();
        if (!Else) return nullptr;

        return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
    }

    /// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
    std::unique_ptr<ExprAST> ParseForExpr()
    {
        getNextToken();// eat the for.

        if (lexer.current_token() != tok_identifier) return LogError("expected identifier after for");

        std::string IdName = IdentifierStr;
        getNextToken();// eat identifier.

        if (lexer.current_token() != tok_equal) return LogError("expected '=' after for");
        getNextToken();// eat '='.

        auto Start = ParseExpression();
        if (!Start) return nullptr;
        if (lexer.current_token() != tok_comma) return LogError("expected ',' after for start value");
        getNextToken();

        auto End = ParseExpression();
        if (!End) return nullptr;

        // The step value is optional.
        std::unique_ptr<ExprAST> Step;
        if (lexer.current_token() == tok_comma)
        {
            getNextToken();
            Step = ParseExpression();
            if (!Step) return nullptr;
        }

        if (lexer.current_token() != tok_in) return LogError("expected 'in' after for");
        getNextToken();// eat 'in'.

        auto Body = ParseExpression();
        if (!Body) return nullptr;

        return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), std::move(Step), std::move(Body));
    }

    /// varexpr ::= 'var' identifier ('=' expression)?
    //                    (',' identifier ('=' expression)?)* 'in' expression
    std::unique_ptr<ExprAST> ParseVarExpr()
    {
        getNextToken();// eat the var.

        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

        // At least one variable name is required.
        if (lexer.current_token() != tok_identifier) return LogError("expected identifier after var");

        while (true)
        {
            std::string Name = IdentifierStr;
            getNextToken();// eat identifier.

            // Read the optional initializer.
            std::unique_ptr<ExprAST> Init = nullptr;
            if (lexer.current_token() == tok_equal)
            {
                getNextToken();// eat the '='.

                Init = ParseExpression();
                if (!Init) return nullptr;
            }

            VarNames.push_back(std::make_pair(Name, std::move(Init)));

            // End of var list, exit loop.
            if (lexer.current_token() != tok_equal) break;
            getNextToken();// eat the ','.

            if (lexer.current_token() != tok_identifier) return LogError("expected identifier list after var");
        }

        // At this point, we have to have 'in'.
        if (lexer.current_token() != tok_in) return LogError("expected 'in' keyword after 'var'");
        getNextToken();// eat 'in'.

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
        switch (lexer.current_token())
        {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
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
        if (!isascii(lexer.current_token()) || lexer.current_token() == tok_leftbracket
            || lexer.current_token() == tok_comma)
            return ParsePrimary();

        // If this is a unary operator, read it.
        int Opc = lexer.current_token();
        getNextToken();
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
            int BinOp = lexer.current_token();
            getNextToken();// eat binop

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

        switch (lexer.current_token())
        {
        default:
            return LogErrorP("Expected function name in prototype");
        case tok_identifier:
            FnName = IdentifierStr;
            Kind = 0;
            getNextToken();
            break;
        case tok_unary:
            getNextToken();
            if (!isascii(lexer.current_token())) return LogErrorP("Expected unary operator");
            FnName = "unary";
            FnName += lexer.current_token();
            Kind = 1;
            getNextToken();
            break;
        case tok_binary:
            getNextToken();
            if (!isascii(lexer.current_token())) return LogErrorP("Expected binary operator");
            FnName = "binary";
            FnName += lexer.current_token();
            Kind = 2;
            getNextToken();

            // Read the precedence if present.
            if (lexer.current_token() == tok_number)
            {
                if (NumVal < 1 || NumVal > 100) return LogErrorP("Invalid precedence: must be 1..100");
                BinaryPrecedence = static_cast<unsigned>(NumVal);
                getNextToken();
            }
            break;
        }

        if (lexer.current_token() != tok_rightbracket) return LogErrorP("Expected '(' in prototype");

        std::vector<std::string> ArgNames;
        while (getNextToken() == tok_identifier) ArgNames.push_back(IdentifierStr);
        if (lexer.current_token() != tok_rightbracket) return LogErrorP("Expected ')' in prototype");

        // success.
        getNextToken();// eat ')'.

        // Verify right number of names for operator.
        if (Kind && ArgNames.size() != Kind) return LogErrorP("Invalid number of operands for operator");

        return std::make_unique<PrototypeAST>(FnName, ArgNames, Kind != 0, BinaryPrecedence);
    }

    /// definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAST> ParseDefinition()
    {
        getNextToken();// eat def.
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
        getNextToken();// eat extern.
        return ParsePrototype();
    }


    void HandleDefinition()
    {
        if (auto FnAST = ParseDefinition())
        {
            if (auto *FnIR = FnAST->codegen())
            {
                fprintf(stderr, "Read function definition:");
                FnIR->print(errs());
                fprintf(stderr, "\n");
            }
        }
        else
        {
            // Skip token for error recovery.
            getNextToken();
        }
    }

    void HandleExtern()
    {
        if (auto ProtoAST = ParseExtern())
        {
            if (auto *FnIR = ProtoAST->codegen())
            {
                fprintf(stderr, "Read extern: ");
                FnIR->print(errs());
                fprintf(stderr, "\n");
                FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
            }
        }
        else
        {
            // Skip token for error recovery.
            getNextToken();
        }
    }

    void HandleTopLevelExpression()
    {
        // Evaluate a top-level expression into an anonymous function.
        if (auto FnAST = ParseTopLevelExpr()) { FnAST->codegen(); }
        else
        {
            // Skip token for error recovery.
            getNextToken();
        }
    }

    void MainLoop()
    {
        while (true)
        {
            switch (CurTok)
            {
            case tok_eof:
                return;
            case ';':// ignore top-level semicolons.
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
            }
        }
    }
};

#endif
