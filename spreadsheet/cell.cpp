#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>
Cell::Cell(SheetInterface &sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet){}
Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if(text.empty()){
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }else if(text[0] == FORMULA_SIGN && text.size() >= 2){
        try {
            impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
            return;
        }catch(...){
            throw FormulaException("Incorrect formula!");
        }
    }else{
        impl_ = std::make_unique<TextImpl>(text);
        return;
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
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

void Cell::InvalidateCellCache() {
    impl_->InvalidateCache();
}

bool Cell::IsCached() {
    return impl_->HasCache();
}

bool Cell::IsCyclicDependency(const Cell *start, const Position end) const {
    //проверяем ячейки, от которых зависит данная
    for(const auto& referenced_cell_pos : GetReferencedCells()){
        //если в списке есть позиция, которая совпадает с конечной - есть циклическая зависимость
        if(referenced_cell_pos == end){
            return true;
        }

        //Получаем указатель на ячейку. Если ее не существует - создадим новую пустую
        const Cell* current_ref_cell = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));
        if(!current_ref_cell){
            sheet_.SetCell(referenced_cell_pos, "");
            current_ref_cell = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell_pos));
        }

        //если текущая ячейка совпадает с начальной - циклическая зависимость
        if(start == current_ref_cell){
            return true;
        }

        //рекурсивно вызываем для каждой ячейки, на которую ссылаемся.
        if(current_ref_cell->IsCyclicDependency(start, end)){
            return true;
        }

    }
    //циклические зависимости не найдены
    return false;
}
//базовый класс ячейки
std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

bool Cell::Impl::HasCache() const {
    return false;
}

void Cell::Impl::InvalidateCache() {}
//Реализация пустой ячейки
Cell::Value Cell::EmptyImpl::GetValue() const {
    return 0.0;
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

CellType Cell::EmptyImpl::GetCellType() const {
    return CellType::EMPTY_CELL;
}


//реализация текстовой ячейки
Cell::TextImpl::TextImpl(std::string text) : text_(std::move(text)){}

Cell::Value Cell::TextImpl::GetValue() const{
    if(text_[0] == ESCAPE_SIGN){
        return text_.substr(1);
    }else{
        return text_;
    }
}
std::string Cell::TextImpl::GetText() const {
    return text_;
}
CellType Cell::TextImpl::GetCellType() const {
    return CellType::TEXT_CELL;
}

//реализация ячейки с формулой
Cell::FormulaImpl::FormulaImpl(std::string formula, SheetInterface &sheet) :
sheet_(sheet),
formula_ptr_(ParseFormula(std::move(formula)))
{}

CellInterface::Value Cell::FormulaImpl::GetValue() const{
    if(!cache_){
        FormulaInterface::Value value = formula_ptr_->Evaluate(sheet_);
        if(std::holds_alternative<double>(value)){
            if(std::isfinite(std::get<double>(value))){
                return std::get<double>(value);
            }else{
                return FormulaError(FormulaError::Category::Arithmetic);
            }
        }
        return std::get<FormulaError>(value);
    }else{
        return *cache_;
    }
}
std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_ptr_->GetReferencedCells();
}

CellType Cell::FormulaImpl::GetCellType() const {
    return CellType::FORMULA_CELL;
}

bool Cell::FormulaImpl::HasCache() const {
    return cache_.has_value();
}

void Cell::FormulaImpl::InvalidateCache(){
    cache_.reset();
}
