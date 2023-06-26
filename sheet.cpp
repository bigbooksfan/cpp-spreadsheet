#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>

std::ostream& operator<< (std::ostream& out, const CellInterface::Value value ) {
    struct ValuePrinter {
        std::ostream& out;
        void operator() (std::monostate) const {}
        void operator() (const std::string& val) const {
            out << val;
        }
        void operator() (double val) const {
            out << val;
        }
        void operator() (const FormulaError& val) const {
            out << val;
        }
    };
    std::visit(ValuePrinter{out}, value);
    return out;
}

Sheet::~Sheet() { }

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
        return;
    }
    if (!data_.count(pos.row) || !data_.at(pos.row).count(pos.col)) {
        data_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    data_[pos.row][pos.col]->Set(text);
    size_.rows = std::max(pos.row + 1, size_.rows);
    size_.cols = std::max(pos.col + 1, size_.cols);
    rows_.insert(pos.row);
    cols_.insert(pos.col);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
        return nullptr;
    }
    if (!data_.count(pos.row) || !data_.at(pos.row).count(pos.col)) {
        return nullptr;
    }
    return data_.at(pos.row).at(pos.col).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
        return nullptr;
    }
    if (!data_.count(pos.row) || !data_.at(pos.row).count(pos.col)) {
        return nullptr;
    }
    return data_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
        return;
    }
    if (!data_.count(pos.row) || !data_.at(pos.row).count(pos.col)) {
        return;
    }
    if (data_[pos.row].size() > 1) {
        data_[pos.row].erase(data_[pos.row].find(pos.col));
    } else {
        data_.erase(pos.row);
    }
    rows_.erase(pos.row);
    cols_.erase(pos.col);
    size_.rows = rows_.empty() ? 0 : (*rows_.rbegin())+1;
    size_.cols = cols_.empty() ? 0 : (*cols_.rbegin())+1;
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    if (size_.rows == 0 || size_.cols == 0) {
        return;
    }
    for (int i = 0; i < size_.rows; ++i) {
        if (data_.count(i)) {
            bool is_first_col = true;
            for (int j = 0; j < size_.cols; ++j) {
                if (!is_first_col) {
                    output << '\t';
                } else {
                    is_first_col = false;
                }
                if (!data_.at(i).count(j)) {
                    continue;
                }
                output << data_.at(i).at(j)->GetValue();
            }
        }
        else {
            output << '\t';
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    if (size_.rows == 0 || size_.cols == 0) {
        return;
    }
    for (int i = 0; i < size_.rows; ++i) {
        if (data_.count(i)) {
            bool is_first_col = true;
            for (int j = 0; j < size_.cols; ++j) {
                if (!is_first_col) {
                    output << '\t';
                } else {
                    is_first_col = false;
                }
                if (!data_.at(i).count(j)) {
                    continue;
                }
                output << data_.at(i).at(j)->GetText();
            }
        }
        else {
            output << '\t';
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}