#pragma once

#include <set>
#include <unordered_map>

#include "cell.h"
#include "common.h"

class Cell;

class Sheet : public SheetInterface {
private:        // fields
    Size size_;
    std::unordered_map<int, std::unordered_map<int, std::unique_ptr<Cell>>> data_;
    std::set<int> rows_;
    std::set<int> cols_;

public:         // constructors
    Sheet() = default;
    ~Sheet();

public:         // methods
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;
};
