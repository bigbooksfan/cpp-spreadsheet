#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <algorithm>

using namespace std::string_literals;

Cell::Cell(SheetInterface& sheet)
    : impl_(std::make_unique<EmptyImpl>(this))
    , sheet_(sheet) { }

Cell::~Cell() { }

void Cell::Set(std::string& text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>(this);
    }

    switch (text[0]) {
    case '\'' :
        impl_ = std::make_unique<TextImpl>(text, this);
        break;
    case '=' :
        if (text.size() > 1) {
            auto formula = ParseFormula(text.substr(1));
            std::vector<Position> referenced_cells = formula->GetReferencedCells();
            std::unordered_set<Cell*> ref_cells;
            std::for_each(referenced_cells.begin(), referenced_cells.end(), [&](const Position pos) {
                ref_cells.insert(dynamic_cast<Cell*>(sheet_.GetCell(pos)));
            });
            if (ref_cells.count(this)) {
                throw CircularDependencyException(""s);
            }
            ref_cells.insert(this);
            for (const Cell* cell : ref_cells) {
                std::vector<std::unordered_set<Cell*>> cells_stack;
                cells_stack.push_back(ref_cells);
                if (cell && cell->impl_ && cell->IsCircular(cells_stack)) {
                    throw CircularDependencyException(""s);
                }
            }
            impl_ = std::make_unique<FormulaImpl>(std::move(formula), sheet_, this);
            AddChilds(referenced_cells);
        } else {
            impl_ = std::make_unique<TextImpl>(text, this);
        }
        break;
    default:
        impl_ = std::make_unique<TextImpl>(text, this);
        break;
    }
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::AddParent(Cell *cell) {
    parents_.insert(cell);
}

void Cell::EraseParent(Cell *parent) {
    if (impl_ && !parents_.empty() && parents_.count(parent)) {
        parents_.erase(parent);
    }
}

void Cell::AddChilds(const std::vector<Position>& new_childs) {
    for (const Position& new_child : new_childs) {
        Cell* child = dynamic_cast<Cell*>(sheet_.GetCell(new_child));
        if (!child) {
            sheet_.SetCell(new_child, ""s);
            child = dynamic_cast<Cell*>(sheet_.GetCell(new_child));
        }
        child->AddParent(this);
        impl_->AddChild(child);
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>(this);
}

void Cell::ClearThisInChilds() {
    impl_->ClearThisInChilds();
}

void Cell::InvalidateCache() {
    impl_->InvalidateCache();
}

bool Cell::IsCircular(std::vector<std::unordered_set<Cell*>>& cells_stack) const {
    return impl_->IsCircular(cells_stack);
}

Cell::Impl::Impl(Cell* cell) : this_cell_(cell) { }

void Cell::Impl::EraseParent(Cell* parent) {
    this_cell_->EraseParent(parent);
}

Cell::EmptyImpl::EmptyImpl(Cell* cell) : Impl(cell) { }

Cell::EmptyImpl::~EmptyImpl() {
    InvalidateCache();
}

CellInterface::Value Cell::EmptyImpl::GetValue() {
    return 0.0;
}

std::string Cell::EmptyImpl::GetText() {
    return ""s;
}

void Cell::EmptyImpl::InvalidateCache() {
    for (Cell* parent : this_cell_->parents_) {
        parent->InvalidateCache();
    }
}

Cell::TextImpl::TextImpl(std::string text, Cell* cell) : Impl(cell), value_(text) { }

Cell::TextImpl::~TextImpl() {
    InvalidateCache();
}

CellInterface::Value Cell::TextImpl::GetValue() {
    return value_[0] == '\'' ? value_.substr(1) : value_;
}

std::string Cell::TextImpl::GetText() {
    return value_;
}

void Cell::TextImpl::InvalidateCache() {
    for (Cell* parent : this_cell_->parents_) {
        parent->InvalidateCache();
    }
}

Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface>&& value
    , SheetInterface& sheet
    , Cell* cell)
    : Impl(cell), value_(std::move(value)), sheet_(sheet) { }

Cell::FormulaImpl::~FormulaImpl() {
    ClearThisInChilds();
    InvalidateCache();
}

CellInterface::Value Cell::FormulaImpl::GetValue() {
    if (!cache_) {
        FormulaInterface::Value result = value_->Evaluate(sheet_);
        cache_ = std::holds_alternative<double>(result) ? Value(std::get<double>(result))
                                                        : Value(std::get<FormulaError>(result));
    }
    return *cache_;
}

std::string Cell::FormulaImpl::GetText() {
    return '=' + value_->GetExpression();
}

void Cell::FormulaImpl::InvalidateCache() {
    cache_ = std::nullopt;
    for (Cell* parent : this_cell_->parents_) {
        if (parent) parent->InvalidateCache();
    }
}

void Cell::FormulaImpl::ClearThisInChilds() {
    for (Cell* child : childs_) {
        if (child->impl_) child->EraseParent(this_cell_);
    }
}

void Cell::FormulaImpl::AddChild(Cell *cell) {
    childs_.insert(cell);
}

bool Cell::FormulaImpl::IsCircular(std::vector<std::unordered_set<Cell*>>& cells_stack) const {
    for (Cell* child : childs_) {
        for (auto cells_set : cells_stack) {
            if (cells_set.count(child)) {
                return true;
            }
        }
    }
    cells_stack.push_back(childs_);
    for (Cell* child : childs_) {
        if (child->IsCircular(cells_stack)) {
            return true;
        }
    }
    cells_stack.pop_back();
    return false;
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return value_->GetReferencedCells();
}