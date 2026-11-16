// group-77
#include "grp_77_ThesisRecord.h"
#include "grp_77_Exceptions.h"
#include <utility>

ThesisRecord::ThesisRecord(std::string registrationTerm)
    : registrationTerm_(std::move(registrationTerm)) {}

void ThesisRecord::submitProgress(const std::string& term, const Grade& progress) {
    // Check state before checking term, so grades after FINAL/TERMINATED report
    // the corresponding lifecycle error.
    if (state_ == ThesisState::Terminated) {
        throw ThesisTerminatedException("No progress can be submitted after termination");
    }
    if (state_ == ThesisState::Final) {
        throw GradeAlreadyFinalException("No progress can be submitted after conversion");
    }
    if (progress.symbol() != "S" && progress.symbol() != "X") {
        throw GradeTypeMismatchException("Thesis progress must be S or X");
    }
    if (term < registrationTerm_ || (!history_.empty() && term <= history_.back().term)) {
        throw InvalidThesisTransitionException("Progress term must be ordered and on/after start");
    }

    ThesisState next = ThesisState::Registered;
    if (progress.symbol() == "S") {
        next = ThesisState::InProgress;
    } else if (state_ == ThesisState::AtRisk) {
        // X after a previous X, with no intervening S.
        next = ThesisState::Terminated;
    } else {
        next = ThesisState::AtRisk;
    }

    // No state is modified until storage has been successfully allocated.
    history_.push_back({term, progress.symbol()});
    state_ = next;
}

void ThesisRecord::convert(const Grade& letter) {
    if (state_ == ThesisState::Terminated) {
        throw ThesisTerminatedException("Cannot convert a terminated thesis");
    }
    if (state_ == ThesisState::Final) {
        throw GradeAlreadyFinalException("Thesis has already been converted");
    }
    if (!letter.isLetter()) {
        throw GradeTypeMismatchException("CONVERT expects an A-through-F letter");
    }
    if (state_ != ThesisState::InProgress) {
        throw InvalidThesisTransitionException("Only an IN_PROGRESS thesis can be converted");
    }

    auto nextGrade = std::make_shared<const Grade>(letter);
    finalGrade_.swap(nextGrade);
    state_ = ThesisState::Final;
}
