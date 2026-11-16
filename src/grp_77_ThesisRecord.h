// group-77
#pragma once

#include "grp_77_Grade.h"
#include <memory>
#include <string>
#include <vector>

enum class ThesisState { Registered, InProgress, AtRisk, Terminated, Final };

struct ThesisProgress {
    std::string term;
    std::string symbol;
};

// All thesis state changes are centralized here, rather than in the command loop.
class ThesisRecord {
public:
    explicit ThesisRecord(std::string registrationTerm);

    ThesisState state() const noexcept { return state_; }
    const Grade* finalGrade() const noexcept { return finalGrade_.get(); }
    const std::vector<ThesisProgress>& history() const noexcept { return history_; }

    void submitProgress(const std::string& term, const Grade& progress);
    void convert(const Grade& letter);
    // Called only after the Registrar validates a DEAN grade correction.
    void replaceFinalGrade(std::shared_ptr<const Grade> replacement) noexcept {
        finalGrade_.swap(replacement);
    }

private:
    std::string registrationTerm_;
    ThesisState state_ = ThesisState::Registered;
    std::vector<ThesisProgress> history_;
    std::shared_ptr<const Grade> finalGrade_;
};
