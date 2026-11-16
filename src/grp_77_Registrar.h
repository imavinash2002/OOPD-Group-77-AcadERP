// group-77
#pragma once

#include "grp_77_Course.h"
#include "grp_77_Student.h"
#include "grp_77_StudentAllocator.h"
#include <map>
#include <cstddef>
#include <optional>
#include <memory>
#include <string>
#include <vector>

enum class EligibilityIssue { AlreadyPassed, DuplicateReg, Prereq, Antireq,
                              CreditLimit, ThesisNotAllowed };

struct ProgramRules {
    int total = 160;
    int elective = 24;
    int minGpa100 = 500;
    std::string thesisMinGrade = "C";
    int thesisMinCredits = 100;
    int thesisMinGpa100 = 600;
    int maxLoad = 24;
};

struct AuditRecord {
    std::string roll;
    std::string description;
};

class Registrar {
public:
    void configure(const std::vector<std::string>& args);
    void addCourse(const std::vector<std::string>& args);
    void addStudent(const std::string& roll, const std::string& name);
    void registerCourse(const std::string& roll, const std::string& courseCode,
                        const std::string& term);
    void submitGrade(const std::string& roll, const std::string& courseCode,
                     const std::string& term, const std::string& symbol);
    void convertThesis(const std::string& roll, const std::string& courseCode,
                       const std::string& letter);
    std::optional<EligibilityIssue> eligibilityIssue(const std::string& roll,
             const std::string& courseCode, const std::string& term) const;
    static std::string issueCode(EligibilityIssue issue);
    std::vector<std::string> graduationFailures(const std::string& roll) const;
    int gpaHundredths(const std::string& roll) const;
    std::string averageGrade(const std::string& roll) const;
    std::string transcript(const std::string& roll) const;
    static std::string formatHundredths(int scaled);

    // Optional bonus commands. Paths are single tokens, relative to cwd.
    void save(const std::string& path) const;
    void load(const std::string& path);
    void changeGrade(const std::string& roll, const std::string& courseCode,
                     const std::string& term, const std::string& symbol,
                     const std::string& deanToken, const std::string& reason);
    std::string audit(const std::string& roll) const;
    void removeStudent(const std::string& roll, const std::string& deanToken,
                       const std::string& reason);
    std::size_t studentCount() const noexcept { return students_.size(); }
    std::size_t studentCapacity() const noexcept { return allocator_->capacity(); }

private:
    const Student& requireStudent(const std::string& roll) const;
    Student& requireStudent(const std::string& roll);
    const Course& requireCourse(const std::string& code) const;

    int earnedCredits(const Student& student) const;
    bool passedAtLeast(const Student& student, const Course& course,
                       const std::string& minimum) const;

    void replayEvent(const std::string& event);
    void writeAuditLogSnapshot() const;
    static std::string eventLine(const std::string& command,
                                 const std::vector<std::string>& args);
    static void authorize(const std::string& token, const std::string& reason);

    ProgramRules rules_;
    std::map<std::string, std::unique_ptr<Course>> courses_;
    // All StudentHandles must die before the allocator, so allocator_ is above students_.
    std::unique_ptr<StudentAllocator> allocator_ = std::make_unique<StudentAllocator>();
    std::map<std::string, StudentHandle> students_;
    std::vector<std::string> journal_;
    std::vector<AuditRecord> auditLog_;
    bool replaying_ = false;
};
