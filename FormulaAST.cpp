#include "FormulaAST.h"

#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>

namespace ASTImpl {

    void Expr::PrintFormula(std::ostream& out, ExprPrecedence parent_precedence, bool right_child = false) const {
        auto precedence = GetPrecedence();
        auto mask = right_child ? PR_RIGHT : PR_LEFT;
        bool parens_needed = PRECEDENCE_RULES[parent_precedence][precedence] & mask;
        if (parens_needed) {
            out << '(';
        }
        DoPrintFormula(out, precedence);
        if (parens_needed) {
            out << ')';
        }
    }

    BinaryOpExpr::BinaryOpExpr(Type type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
            : type_(type), lhs_(std::move(lhs)), rhs_(std::move(rhs)) { }

    void BinaryOpExpr::Print(std::ostream& out) const {
        out << '(' << static_cast<char>(type_) << ' ';
        lhs_->Print(out);
        out << ' ';
        rhs_->Print(out);
        out << ')';
    }

    void BinaryOpExpr::DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const {
        lhs_->PrintFormula(out, precedence);
        out << static_cast<char>(type_);
        rhs_->PrintFormula(out, precedence, /* right_child = */ true);
    }

    ExprPrecedence BinaryOpExpr::GetPrecedence() const {
        switch (type_) {
        case Add:
            return EP_ADD;
        case Subtract:
            return EP_SUB;
        case Multiply:
            return EP_MUL;
        case Divide:
            return EP_DIV;
        default:
            assert(false);
            return static_cast<ExprPrecedence>(INT_MAX);
        }
    }

    double BinaryOpExpr::Evaluate(const std::function<double(Position)>& get_cell_value) const {
        double rhs = rhs_->Evaluate(std::ref(get_cell_value));
        double lhs = lhs_->Evaluate(std::ref(get_cell_value));
        switch (type_) {
        case Add:
            if ((lhs > 0 && rhs > 0 && rhs > std::numeric_limits<double>::max() - lhs)
                || (rhs < 0 && lhs < 0 && lhs < std::numeric_limits<double>::min() - rhs)) {
                throw FormulaError(FormulaError::Category::Div0);
            }
            return lhs + rhs;
        case Subtract:
            if ((lhs < 0 && rhs > 0 && std::numeric_limits<double>::min() + rhs > lhs)
                || (lhs > 0 && rhs < 0 && std::numeric_limits<double>::max() + rhs < lhs)) {
                throw FormulaError(FormulaError::Category::Div0);
            }
            return lhs - rhs;
        case Multiply:
            if (std::abs(lhs) > INACCURACY && rhs > std::numeric_limits<double>::max() / std::abs(lhs)) {
                throw FormulaError(FormulaError::Category::Div0);
            }
            return lhs * rhs;
        case Divide:
            if (std::abs(rhs) < INACCURACY) {
                throw FormulaError(FormulaError::Category::Div0);
            }
            else {
                return lhs_->Evaluate(std::ref(get_cell_value)) / rhs;
            }
        default:
            assert(false);
            return 0;
        }
    }

    UnaryOpExpr::UnaryOpExpr(Type type, std::unique_ptr<Expr> operand) 
        : type_(type), operand_(std::move(operand)) { }

        void UnaryOpExpr::Print(std::ostream& out) const {
        out << '(' << static_cast<char>(type_) << ' ';
        operand_->Print(out);
        out << ')';
    }

    void UnaryOpExpr::DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const {
        out << static_cast<char>(type_);
        operand_->PrintFormula(out, precedence);
    }

    ExprPrecedence UnaryOpExpr::GetPrecedence() const {
        return EP_UNARY;
    }

    double UnaryOpExpr::Evaluate(const std::function<double(Position)>& get_cell_value) const {
        switch (type_) {
        case Type::UnaryPlus:
            return operand_->Evaluate(std::ref(get_cell_value));
        case Type::UnaryMinus:
            return -(operand_->Evaluate(std::ref(get_cell_value)));
        default:
            assert(false);
            return 0;
        }
    }

    CellExpr::CellExpr(const Position* cell) : cell_(cell) { }

    void CellExpr::Print(std::ostream& out) const {
        if (!cell_->IsValid()) {
            out << FormulaError::Category::Ref;
        }
        else {
            out << cell_->ToString();
        }
    }

    void CellExpr::DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const {
        Print(out);
    }

    ExprPrecedence CellExpr::GetPrecedence() const {
        return EP_ATOM;
    }

    double CellExpr::Evaluate(const std::function<double(Position)>& get_cell_value) const {
        return get_cell_value(*cell_);
    }

    NumberExpr::NumberExpr(double value) : value_(value) { }

    void NumberExpr::Print(std::ostream& out) const {
        out << value_;
    }

    void NumberExpr::DoPrintFormula(std::ostream& out, ExprPrecedence precedence) const {
        out << value_;
    }

    ExprPrecedence NumberExpr::GetPrecedence() const {
        return EP_ATOM;
    }

    double NumberExpr::Evaluate(const std::function<double(Position)>&) const {
        return value_;
    }

    std::unique_ptr<Expr> ParseASTListener::MoveRoot() {
        assert(args_.size() == 1);
        auto root = std::move(args_.front());
        args_.clear();

        return root;
    }

    std::forward_list<Position> ParseASTListener::MoveCells() {
        return std::move(cells_);
    }

    void ParseASTListener::exitUnaryOp(FormulaParser::UnaryOpContext* ctx) {
        assert(args_.size() >= 1);

        auto operand = std::move(args_.back());

        UnaryOpExpr::Type type;
        if (ctx->SUB()) {
            type = UnaryOpExpr::UnaryMinus;
        } else {
            assert(ctx->ADD() != nullptr);
            type = UnaryOpExpr::UnaryPlus;
        }

        auto node = std::make_unique<UnaryOpExpr>(type, std::move(operand));
        args_.back() = std::move(node);
    }


    void ParseASTListener::exitLiteral(FormulaParser::LiteralContext* ctx) {
        double value = 0;
        auto valueStr = ctx->NUMBER()->getSymbol()->getText();
        std::istringstream in(valueStr);
        in >> value;
        if (!in) {
            throw ParsingError("Invalid number: " + valueStr);
        }

        auto node = std::make_unique<NumberExpr>(value);
        args_.push_back(std::move(node));
    }

    void ParseASTListener::exitCell(FormulaParser::CellContext* ctx) {
        auto value_str = ctx->CELL()->getSymbol()->getText();
        auto value = Position::FromString(value_str);
        if (!value.IsValid()) {
            throw FormulaException("Invalid position: " + value_str);
        }

        cells_.push_front(value);
        auto node = std::make_unique<CellExpr>(&cells_.front());
        args_.push_back(std::move(node));
    }

    void ParseASTListener::exitBinaryOp(FormulaParser::BinaryOpContext* ctx) {
        assert(args_.size() >= 2);

        auto rhs = std::move(args_.back());
        args_.pop_back();

        auto lhs = std::move(args_.back());

        BinaryOpExpr::Type type;
        if (ctx->ADD()) {
            type = BinaryOpExpr::Add;
        } else if (ctx->SUB()) {
            type = BinaryOpExpr::Subtract;
        } else if (ctx->MUL()) {
            type = BinaryOpExpr::Multiply;
        } else {
            assert(ctx->DIV() != nullptr);
            type = BinaryOpExpr::Divide;
        }

        auto node = std::make_unique<BinaryOpExpr>(type, std::move(lhs), std::move(rhs));
        args_.back() = std::move(node);
    }

    void ParseASTListener::visitErrorNode(antlr4::tree::ErrorNode* node) {
        throw ParsingError("Error when parsing: " + node->getSymbol()->getText());
    }

    void BailErrorListener::syntaxError(
        antlr4::Recognizer* recognizer
        , antlr4::Token* offendingSymbol
        , size_t line
        , size_t charPositionInLine
        , const std::string& msg,
        std::exception_ptr e) {
        throw ParsingError("Error when lexing: " + msg);
    }

}  // namespace ASTImpl

FormulaAST ParseFormulaAST(std::istream& in) try {
    using namespace antlr4;

    ANTLRInputStream input(in);

    FormulaLexer lexer(&input);
    ASTImpl::BailErrorListener error_listener;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&error_listener);

    CommonTokenStream tokens(&lexer);

    FormulaParser parser(&tokens);
    auto error_handler = std::make_shared<BailErrorStrategy>();
    parser.setErrorHandler(error_handler);
    parser.removeErrorListeners();

    tree::ParseTree* tree = parser.main();
    ASTImpl::ParseASTListener listener;
    tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    return FormulaAST(listener.MoveRoot(), listener.MoveCells());
} catch (...) {
    throw FormulaException("");
}

FormulaAST ParseFormulaAST(const std::string& in_str) {
    std::istringstream in(in_str);
    return ParseFormulaAST(in);
}

void FormulaAST::PrintCells(std::ostream& out) const {
    for (auto cell : cells_) {
        out << cell.ToString() << ' ';
    }
}

void FormulaAST::Print(std::ostream& out) const {
    root_expr_->Print(out);
}

void FormulaAST::PrintFormula(std::ostream& out) const {
    root_expr_->PrintFormula(out, ASTImpl::EP_ATOM);
}

double FormulaAST::Execute(const std::function<double(Position)>& get_cell_value) const {
    return root_expr_->Evaluate(std::ref(get_cell_value));
}

FormulaAST::FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr, std::forward_list<Position> cells)
    : root_expr_(std::move(root_expr))
    , cells_(std::move(cells)) {
    cells_.sort();  // to avoid sorting in GetReferencedCells
}

FormulaAST::~FormulaAST() = default;
