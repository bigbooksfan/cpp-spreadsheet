#pragma once 

#include <string> 
#include <vector> 

#include "common.h"
#include "FormulaAST.h"

class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    FormulaInterface() = default;
    virtual ~FormulaInterface() = default;
    virtual Value Evaluate(const SheetInterface& sheet) const = 0;
    virtual std::string GetExpression() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Formula : public FormulaInterface {
private:        // fields
    FormulaAST ast_;

public:         // constructors
    explicit Formula(std::string expression);

public:         // methods
    Value Evaluate(const SheetInterface& sheet) const override;
    std::string GetExpression() const override;
    std::vector<Position> GetReferencedCells() const override;
};

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);