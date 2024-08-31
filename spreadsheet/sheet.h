#pragma once

#include "cell.h"
#include "common.h"
#include <map>
#include <set>
#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами
    void InvalidateCell(Position pos);
    void AddDependCell(const Position ref_cell, const Position depend_cell);
    const std::set<Position> GetDependentCells(const Position pos);
    void RemoveDependency(const Position pos);

private:
    // Можете дополнить ваш класс нужными полями и методами
    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);
        }
    };

    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> sheet_;
    std::map<Position, std::set<Position>> cells_dependencies_;
};