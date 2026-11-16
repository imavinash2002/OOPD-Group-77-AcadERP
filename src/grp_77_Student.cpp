// group-77
#include "grp_77_Student.h"
#include "grp_77_Exceptions.h"
#include <utility>

Attempt::Attempt(std::string courseCode, std::string term, bool isThesis,
                 bool isImprovement)
    : courseCode_(std::move(courseCode)), term_(std::move(term)),
      isImprovement_(isImprovement) {
    if (isThesis) thesis_ = std::make_unique<ThesisRecord>(term_);
}

const Grade* Attempt::grade() const noexcept {
    return thesis_ ? thesis_->finalGrade() : grade_.get();
}

bool Attempt::hasFinalResult() const noexcept {
    if (thesis_) {
        return thesis_->state() == ThesisState::Final ||
               thesis_->state() == ThesisState::Terminated;
    }
    return grade_ != nullptr && grade_->isFinal();
}

void Attempt::submitGrade(const Grade& grade) {
    if (thesis_) throw GradeTypeMismatchException("Thesis uses progress grades");
    if (grade_ != nullptr && grade_->isFinal()) {
        throw GradeAlreadyFinalException("Cannot change an already-final grade");
    }
    if (grade_ != nullptr && grade.symbol() == "I") {
        throw GradeAlreadyFinalException("Incomplete may only be replaced by a final grade");
    }
    auto nextGrade = std::make_shared<const Grade>(grade);
    grade_.swap(nextGrade);
}

void Attempt::submitThesisProgress(const std::string& term, const Grade& progress) {
    if (!thesis_) throw GradeTypeMismatchException("Not a thesis attempt");
    thesis_->submitProgress(term, progress);
}

void Attempt::convertThesis(const Grade& letter) {
    if (!thesis_) throw GradeTypeMismatchException("Not a thesis attempt");
    thesis_->convert(letter);
}

void Attempt::replaceFinalGrade(std::shared_ptr<const Grade> replacement) noexcept {
    if (thesis_) thesis_->replaceFinalGrade(std::move(replacement));
    else grade_.swap(replacement);
}

Student::Student(std::string roll, std::string name)
    : roll_(std::move(roll)), name_(std::move(name)) {}

void Student::addAttempt(const std::string& courseCode, const std::string& term,
                         bool isThesis, bool isImprovement) {
    // Construct the new attempt before modifying the vector (strong guarantee).
    Attempt next(courseCode, term, isThesis, isImprovement);
    attempts_.push_back(std::move(next));
}

Attempt* Student::findAttempt(const std::string& courseCode, const std::string& term) noexcept {
    for (Attempt& attempt : attempts_) {
        if (attempt.courseCode() == courseCode && attempt.term() == term) return &attempt;
    }
    return nullptr;
}

Attempt* Student::latestThesisAttempt(const std::string& courseCode) noexcept {
    Attempt* latest = nullptr;
    for (Attempt& attempt : attempts_) {
        if (attempt.courseCode() == courseCode && attempt.isThesis() &&
            (latest == nullptr || attempt.term() > latest->term())) {
            latest = &attempt;
        }
    }
    return latest;
}

const Attempt* Student::countingAttempt(const std::string& courseCode) const noexcept {
    const Attempt* selected = nullptr;
    for (const Attempt& attempt : attempts_) {
        if (attempt.courseCode() != courseCode || !attempt.hasFinalResult()) continue;
        if (selected == nullptr || attempt.term() > selected->term()) selected = &attempt;
    }
    return selected;
}

bool Student::hasOpenAttempt(const std::string& courseCode) const noexcept {
    for (const Attempt& attempt : attempts_) {
        if (attempt.courseCode() == courseCode && attempt.isOpen()) return true;
    }
    return false;
}

bool Student::hasUsedImprovement(const std::string& courseCode) const noexcept {
    for (const Attempt& attempt : attempts_) {
        if (attempt.courseCode() == courseCode && attempt.isImprovement()) return true;
    }
    return false;
}
