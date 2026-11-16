// group-77
#pragma once

#include "grp_77_Grade.h"
#include "grp_77_ThesisRecord.h"
#include <memory>
#include <string>
#include <vector>

class Attempt {
public:
    Attempt(std::string courseCode, std::string term, bool isThesis = false,
            bool isImprovement = false);
    const std::string& courseCode() const noexcept { return courseCode_; }
    const std::string& term() const noexcept { return term_; }
    const Grade* grade() const noexcept;
    const ThesisRecord* thesis() const noexcept { return thesis_.get(); }
    bool isThesis() const noexcept { return thesis_ != nullptr; }
    bool isImprovement() const noexcept { return isImprovement_; }
    bool hasFinalResult() const noexcept;
    bool isOpen() const noexcept { return !hasFinalResult(); }

    void submitGrade(const Grade& grade);
    void submitThesisProgress(const std::string& term, const Grade& progress);
    void convertThesis(const Grade& letter);
    void replaceFinalGrade(std::shared_ptr<const Grade> replacement) noexcept;

private:
    std::string courseCode_;
    std::string term_;
    std::shared_ptr<const Grade> grade_;
    std::unique_ptr<ThesisRecord> thesis_;
    bool isImprovement_ = false;
};

class Student {
public:
    Student(std::string roll, std::string name);
    const std::string& roll() const noexcept { return roll_; }
    const std::string& name() const noexcept { return name_; }
    const std::vector<Attempt>& attempts() const noexcept { return attempts_; }
    void addAttempt(const std::string& courseCode, const std::string& term,
                    bool isThesis = false, bool isImprovement = false);
    Attempt* findAttempt(const std::string& courseCode, const std::string& term) noexcept;
    Attempt* latestThesisAttempt(const std::string& courseCode) noexcept;
    const Attempt* countingAttempt(const std::string& courseCode) const noexcept;
    bool hasOpenAttempt(const std::string& courseCode) const noexcept;
    bool hasUsedImprovement(const std::string& courseCode) const noexcept;
private:
    std::string roll_;
    std::string name_;
    std::vector<Attempt> attempts_;
};
