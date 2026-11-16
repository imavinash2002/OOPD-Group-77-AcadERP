// group-77
#pragma once

#include <string>

// Immutable, validated grade value. Create it only through Grade::create().
class Grade final {
public:
    static Grade create(const std::string& symbol);
    const std::string& symbol() const noexcept { return symbol_; }
    int points() const noexcept { return points_; }  // -1 for S, X, I.
    bool isPass() const noexcept { return passed_; }
    bool isFinal() const noexcept { return symbol_ != "I"; }
    bool isLetter() const noexcept { return points_ >= 0; }
private:
    Grade(std::string symbol, int points, bool passed)
        : symbol_(std::move(symbol)), points_(points), passed_(passed) {}
    const std::string symbol_;
    const int points_;
    const bool passed_;
};
