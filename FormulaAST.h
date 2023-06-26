#pragma once


#include <forward_list>
#include <functional>
#include <stdexcept>

#include "FormulaBaseListener.h"
#include "FormulaLexer.h"
#include "FormulaParser.h"
#include "common.h"

namespace ASTImpl {
    class Expr;

    enum ExprPrecedence {
        EP_ADD,
        EP_SUB,
        EP_MUL,
        EP_DIV,
        EP_UNARY,
        EP_ATOM,
        EP_END,
    };

    enum PrecedenceRule {
        PR_NONE = 0b00,                // never needed
        PR_LEFT = 0b01,                // needed for a left child
        PR_RIGHT = 0b10,               // needed for a right child
        PR_BOTH = PR_LEFT | PR_RIGHT,  // needed for both children
    };

    constexpr PrecedenceRule PRECEDENCE_RULES[EP_END][EP_END] = {
        /* EP_ADD */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_SUB */ {PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_MUL */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_DIV */ {PR_BOTH, PR_BOTH, PR_RIGHT, PR_RIGHT, PR_NONE, PR_NONE},
        /* EP_UNARY */ {PR_BOTH, PR_BOTH, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
        /* EP_ATOM */ {PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE, PR_NONE},
    };

    const double INACCURACY = 10e-6;

    class Expr {
    public:         // constructors
        virtual ~Expr() = default;

    public:         // methods
        virtual void Print(std::ostream& out) const = 0;
        virtual void DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const = 0;
        virtual double Evaluate(const std::function<double(Position)>& get_cell_value) const = 0;
        virtual ExprPrecedence GetPrecedence() const = 0;
        void PrintFormula(std::ostream& out, ExprPrecedence parent_precedence, bool right_child) const;
    };

    class BinaryOpExpr final : public Expr {
    public:         // fields
        enum Type : char {
            Add = '+',
            Subtract = '-',
            Multiply = '*',
            Divide = '/',
        };

    private:        // fields
        Type type_;
        std::unique_ptr<Expr> lhs_;
        std::unique_ptr<Expr> rhs_;

    public:         // constructors
        explicit BinaryOpExpr(Type type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);
            
    public:         // methods
        void Print(std::ostream& out) const override;
        void DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const override;
        ExprPrecedence GetPrecedence() const;
        double Evaluate(const std::function<double(Position)>& get_cell_value) const override;
    };

    class UnaryOpExpr final : public Expr {
    public:         // fields
        enum Type : char {
            UnaryPlus = '+',
            UnaryMinus = '-',
        };

    private:        // fields
        Type type_;
        std::unique_ptr<Expr> operand_;

    public:         // constructors
        explicit UnaryOpExpr(Type type, std::unique_ptr<Expr> operand);

    public:         // methods
        void Print(std::ostream& out) const override;
        void DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const override;
        ExprPrecedence GetPrecedence() const override;
        double Evaluate(const std::function<double(Position)>& get_cell_value) const override;
    };

    class CellExpr final : public Expr {
    private:        // fields
        const Position* cell_;

    public:         // constructors
        explicit CellExpr(const Position* cell);

    public:         // methods
        void Print(std::ostream& out) const override;
        void DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const override;
        ExprPrecedence GetPrecedence() const override;
        double Evaluate(const std::function<double(Position)>& get_cell_value) const override;
    };

    class NumberExpr final : public Expr {
    private:        // fields
        double value_;

    public:         // constructors
        explicit NumberExpr(double value);

    public:         // methods
        void Print(std::ostream& out) const override;
        void DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const override;
        ExprPrecedence GetPrecedence() const override;
        double Evaluate(const std::function<double(Position)>&) const override;
    };

    class ParseASTListener final : public FormulaBaseListener {
    private:        // fields
        std::vector<std::unique_ptr<Expr>> args_;
        std::forward_list<Position> cells_;

    public:         // methods
        std::unique_ptr<Expr> MoveRoot();
        std::forward_list<Position> MoveCells();

        void exitUnaryOp(FormulaParser::UnaryOpContext* ctx) override;
        void exitLiteral(FormulaParser::LiteralContext* ctx) override;
        void exitCell(FormulaParser::CellContext* ctx) override;
        void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override;
        void visitErrorNode(antlr4::tree::ErrorNode* node) override;
    };

    class BailErrorListener : public antlr4::BaseErrorListener {
    public:
        void syntaxError(
            antlr4::Recognizer* recognizer
            , antlr4::Token* offendingSymbol
            , size_t line
            , size_t charPositionInLine
            , const std::string& msg,
            std::exception_ptr e) override;
    };
}       // namespace ASTImpl 

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
private:        // fields 
    std::unique_ptr<ASTImpl::Expr> root_expr_;
    std::forward_list<Position> cells_;

public:         // constructors 
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr, std::forward_list<Position> cells);
    FormulaAST(FormulaAST&&) = default;
    FormulaAST& operator=(FormulaAST&&) = default;
    ~FormulaAST();

public:         // methods 
    double Execute(const std::function<double(Position)>& get_cell_value) const;
    void PrintCells(std::ostream& out) const;
    void Print(std::ostream& out) const;
    void PrintFormula(std::ostream& out) const;
    
    std::forward_list<Position>& GetCells() { return cells_; }
    const std::forward_list<Position>& GetCells() const { return cells_; }
};

FormulaAST ParseFormulaAST(std::istream& in);
FormulaAST ParseFormulaAST(const std::string& in_str);