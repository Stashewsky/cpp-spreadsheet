#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression):
        ast_(ParseFormulaAST(std::move(expression))),
        referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end())
        {}

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                std::function<double(Position)> args = [&sheet](const Position pos) {
                    if (pos.IsValid()) {
                        const auto *cell = sheet.GetCell(pos);

                        if (!cell) {
                            return 0.0;
                        }

                        auto value = sheet.GetCell(pos)->GetValue();

                        if (std::holds_alternative<double>(value)) {
                            return std::get<double>(value);
                        } else if (std::holds_alternative<std::string>(value)) {
                            try {
                                std::string str_val = std::get<std::string>(value);
                                if (str_val.empty()) {
                                    return 0.0;
                                } else if (std::all_of(str_val.begin(), str_val.end(),
                                                       [](char c) {
                                                           return (std::isdigit(c) || c == '.');
                                                       })) {
                                    return std::stod(str_val);
                                } else {
                                    throw FormulaError(FormulaError::Category::Value);
                                }
                            } catch (...) {
                                throw FormulaError(FormulaError::Category::Value);
                            }
                        } else {
                            throw std::get<FormulaError>(value);
                        }
                    }else{
                        throw InvalidPositionException("Invalid position!");
                    }
                };
                return ast_.Execute(args);
            }catch (FormulaError& fe) {
                return fe;
            }
        }

        std::string GetExpression() const override{
            std::stringstream str;
            ast_.PrintFormula(str);
            return str.str();
        }

        std::vector<Position> GetReferencedCells() const override{
            std::vector<Position> result;

            std::set<Position> ref_cells(referenced_cells_.begin(), referenced_cells_.end());
            for(const auto& c : ref_cells){
                result.push_back(c);
            }
            return result;
        }

    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try{
        return std::make_unique<Formula>(std::move(expression));
    }catch (const std::exception&){
        throw FormulaException("When parsing formula error occured!");
    }
}