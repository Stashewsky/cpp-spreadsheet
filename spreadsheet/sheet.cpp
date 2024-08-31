#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("You try to SET cell with invalid position!");
    }

    auto current_cell = GetCell(pos);
    // Если ячейка на данной позиции еще не создана, создаем новую ячейку
    if (!current_cell) {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);

        //проверим циклические зависимости
        if(new_cell->IsCyclicDependency(new_cell.get(), pos)){
            throw CircularDependencyException("Circular dependency detected!");
        }

        //добавим зависимости
        for(const auto& referenced_cell : new_cell->GetReferencedCells()){
            AddDependCell(referenced_cell, pos);
        }

        sheet_[pos] = std::move(new_cell);
    }else{
        std::string current_cell_text = current_cell->GetText();
        //очищаем кеш ячейки, и зависимых ячеек.
        // Очищаем содержимое ячейки, и удаляем зависимости
        InvalidateCell(pos);
        RemoveDependency(pos);

        dynamic_cast<Cell*>(current_cell)->Clear();
        dynamic_cast<Cell*>(current_cell)->Set(text);

        if(dynamic_cast<Cell*>(current_cell)->IsCyclicDependency(dynamic_cast<Cell*>(current_cell), pos)){
            dynamic_cast<Cell*>(current_cell)->Set(std::move(current_cell_text));
            throw CircularDependencyException("Circular dependency detected!");
        }

        for(const auto& referenced_cell : dynamic_cast<Cell*>(current_cell)->GetReferencedCells()){
            AddDependCell(referenced_cell, pos);
        }
    }
    sheet_[pos]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
     if(!pos.IsValid()){
         throw InvalidPositionException("You try to GET cell with invalid position!");
     }

     if(sheet_.count(pos)){
         return sheet_.at(pos).get();
     }else{
         return nullptr;
     }
}
CellInterface* Sheet::GetCell(Position pos) {
    if(!pos.IsValid()){
        throw InvalidPositionException("You try to GET cell with invalid position!");
    }

    if(sheet_.count(pos)){
        return sheet_.at(pos).get();
    }else{
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position.");
    }

    if(sheet_.count(pos)){
        sheet_.at(pos).reset();
    }
}

Size Sheet::GetPrintableSize() const {
    Size printable_size;
    for(auto& [pos, cell] : sheet_){
        if(cell && !cell->GetText().empty()){
            printable_size.rows = std::max(printable_size.rows, pos.row + 1);
            printable_size.cols = std::max(printable_size.cols, pos.col + 1);
        }
    }
    return printable_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for(int row = 0; row < printable_size.rows; row++){
        for(int col = 0; col < printable_size.cols; col++){
            if(col > 0){
                output << '\t';
            }

            Position pos{row, col};
            auto it = sheet_.find(pos);
            if(it != sheet_.end()) {
                const CellInterface* cell = it->second.get();
                if (cell) {
                    std::visit(
                            [&output](const auto &value) {
                                output << value;
                            },
                            cell->GetValue());
                }
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for(int row = 0; row < GetPrintableSize().rows; row++){
        for(int col = 0; col < GetPrintableSize().cols; col++){
            if(col > 0){
                output << '\t';
            }

            Position pos{row, col};
            auto it = sheet_.find(pos);
            if(it != sheet_.end()){
                const CellInterface* cell = it->second.get();
                if(cell){
                    output << cell->GetText();
                }
            }
        }
        output << '\n';
    }
}

void Sheet::InvalidateCell(Position pos) {
    for(const auto& dep_cell : GetDependentCells(pos)){
        auto cell = GetCell(dep_cell);
        dynamic_cast<Cell*>(cell)->InvalidateCellCache();
        InvalidateCell(dep_cell);
    }
}

void Sheet::AddDependCell(const Position ref_cell, const Position depend_cell) {
    cells_dependencies_[ref_cell].insert(depend_cell);
}

const std::set<Position> Sheet::GetDependentCells(const Position pos) {
    if(cells_dependencies_.count(pos)){
        return cells_dependencies_.at(pos);
    }else{
        return {};
    }
}

void Sheet::RemoveDependency(const Position pos) {
    cells_dependencies_.erase(pos);
}
std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}