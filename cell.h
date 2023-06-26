#pragma once

#include <functional>
#include <unordered_set>
#include <optional>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
public:     // constructors 
    Cell(SheetInterface& sheet);
    ~Cell();

public:     // methods 
    void Set(std::string& text);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void AddParent(Cell* cell);
    void EraseParent(Cell* parent);
    void AddChilds(const std::vector<Position>& new_childs);

    void Clear();
    void ClearThisInChilds();

    void InvalidateCache();

    bool IsCircular(std::vector<std::unordered_set<Cell*>>& cells_stack) const;

private:        // Implementations 
    class Impl {
    public:    // fields
        Cell* this_cell_;

    public:     // constructors 
        Impl(Cell* cell);
        virtual ~Impl() = default;

    public:     // methods 
        virtual CellInterface::Value GetValue() = 0;
        virtual std::string GetText() = 0;
        virtual void InvalidateCache() = 0;
        void EraseParent(Cell* parent);

        virtual void ClearThisInChilds() { }
        virtual void AddChild(Cell*) = 0;
        virtual bool IsCircular(std::vector<std::unordered_set<Cell*>>& cells_stack) const { return false; }
        virtual std::vector<Position> GetReferencedCells() const { return {}; }
    };

    class EmptyImpl : public Impl {
    public:     // constructors 
        EmptyImpl(Cell* cell);
        ~EmptyImpl();

    public:     // methods 
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        void InvalidateCache() override;
        void AddChild(Cell*) override { }
    };

    class TextImpl : public Impl {
    private:        // fields 
        std::string value_;

    public:         // constructors 
        TextImpl(std::string text, Cell* cell);
        ~TextImpl();

    public:         //methods 
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        void InvalidateCache() override;
        void AddChild(Cell*) override { }
    };

    class FormulaImpl : public Impl {
    private:        // fields 
        std::unique_ptr<FormulaInterface> value_;
        std::optional<Value> cache_;
        SheetInterface& sheet_;
        std::unordered_set<Cell*> childs_;

    public:         // constructors 
        FormulaImpl(std::unique_ptr<FormulaInterface>&& value, SheetInterface& sheet, Cell* cell);
        ~FormulaImpl();

    public:         // methods 
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        void InvalidateCache() override;
        void ClearThisInChilds() override;
        void AddChild(Cell* cell) override;
        bool IsCircular(std::vector<std::unordered_set<Cell*>>& cells_stack) const override;
        std::vector<Position> GetReferencedCells() const override;
    };

private:        // fields 
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    std::unordered_set<Cell*> parents_;

};
