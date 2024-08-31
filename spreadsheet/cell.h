#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <set>
#include <optional>
class Sheet;

enum class CellType{ //типы ячеек
    EMPTY_CELL,
    TEXT_CELL,
    FORMULA_CELL
};

class Cell : public CellInterface {
public:
    Cell(SheetInterface &sheet); //ячейка принимает ссылку на таблицу
    ~Cell();
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    void InvalidateCellCache(); //инвалидация кеша в ячейке
    bool IsCached(); //проверка существования кеша
    bool IsCyclicDependency(const Cell* start, const Position end) const;
private:
    //проверка на циклическую зависимость
    class Impl; //базовый класс реализации ячейки

    std::unique_ptr<Impl> impl_; //указатель на класс-реализацию
    SheetInterface& sheet_; //ссылка на лист таблицы

    class Impl {
    public:
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
        virtual CellType GetCellType() const = 0;
        virtual bool HasCache() const;
        virtual void InvalidateCache();
        virtual ~Impl() = default;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
        CellType GetCellType() const override;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text);
        CellInterface::Value GetValue() const override; //возвращает текст с учетом экранирующих символов
        std::string GetText() const override; //возвращает "сырой" текст
        CellType GetCellType() const override;
    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string formula, SheetInterface &sheet);
        CellInterface::Value GetValue() const override; //возвращает вычисленное значение формулы
        std::string GetText() const override; //возвращает текст самой формулы
        std::vector<Position> GetReferencedCells() const override;
        CellType GetCellType() const override;
        bool HasCache() const override;
        void InvalidateCache() override;
    private:
        SheetInterface &sheet_;
        std::unique_ptr<FormulaInterface> formula_ptr_;
        std::optional<CellInterface::Value> cache_;
    };
};