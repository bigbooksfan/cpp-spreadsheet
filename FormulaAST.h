#pragma once

#include <forward_list>
#include <functional>
#include <stdexcept>

#include "FormulaLexer.h"
#include "common.h"

namespace ASTImpl {
    class Expr;
}       // namespace ASTImpl

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
private:        // fields
    std::unique_ptr<ASTImpl::Expr> root_expr_;
    std::vector<Position> cells_;

public:         // constructors
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr, std::forward_list<Position> cells);
    FormulaAST(FormulaAST&&) = default;
    FormulaAST& operator=(FormulaAST&&) = default;
    ~FormulaAST();

public:         // methods
    std::forward_list<Position>& GetCells();
    const std::forward_list<Position>& GetCells() const;

    double Execute(const std::function<double(Position)>& get_cell_value) const;
    void PrintCells(std::ostream& out) const;
    void Print(std::ostream& out) const;
    void PrintFormula(std::ostream& out) const;
};

FormulaAST ParseFormulaAST(std::istream& in);
FormulaAST ParseFormulaAST(const std::string& in_str);