// group-77
#include "grp_77_Registrar.h"
#include "grp_77_Exceptions.h"
#include "grp_77_CommandParser.h"
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <set>
#include <sstream>
#include <utility>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace {
// Stage the event BEFORE any mutation. If a command fails, the RAII guard
// discards its event; commit is noexcept after the operation succeeds.
class JournalGuard {
public:
    JournalGuard(std::vector<std::string>& journal, std::string event)
        : journal_(journal) { journal_.push_back(std::move(event)); }
    JournalGuard(const JournalGuard&) = delete;
    JournalGuard& operator=(const JournalGuard&) = delete;
    ~JournalGuard() noexcept { if (!committed_) journal_.pop_back(); }
    void commit() noexcept { committed_ = true; }
private:
    std::vector<std::string>& journal_;
    bool committed_ = false;
};

std::string joinReason(const std::vector<std::string>& tokens, std::size_t from) {
    std::string result;
    for (std::size_t i = from; i < tokens.size(); ++i) {
        if (i > from) result += ' ';
        result += tokens[i];
    }
    return result;
}

int parseUnsigned(const std::string& input) {
    if (input.empty()) throw ParseException("Expected a nonnegative integer");
    int result = 0;
    const char* first = input.data();
    const char* last = first + input.size();
    auto parsed = std::from_chars(first, last, result);
    if (parsed.ec != std::errc{} || parsed.ptr != last || result < 0) {
        throw ParseException("Invalid nonnegative integer: " + input);
    }
    return result;
}
int parseHundredths(const std::string& input) {
    const std::size_t dot = input.find('.');
    if (dot == std::string::npos || dot == 0 || input.size() != dot + 3) {
        throw ParseException("Expected GPA in d.dd form");
    }
    for (std::size_t i = dot + 1; i < input.size(); ++i) {
        if (input[i] < '0' || input[i] > '9') throw ParseException("Invalid GPA decimal digits");
    }
    const int integerPart = parseUnsigned(input.substr(0, dot));
    if (integerPart > 10) throw ParseException("GPA threshold above 10.00");
    const int fraction = (input[dot + 1] - '0') * 10 + (input[dot + 2] - '0');
    const int scaled = integerPart * 100 + fraction;
    if (scaled > 1000) throw ParseException("GPA threshold above 10.00");
    return scaled;
}
void requireValidTerm(const std::string& term) {
    bool valid = term.size() == 6 && term[4] == '-' &&
                 term[5] >= '1' && term[5] <= '3';
    for (std::size_t i = 0; i < 4 && valid; ++i) {
        valid = term[i] >= '0' && term[i] <= '9';
    }
    if (!valid) throw ParseException("Invalid term: " + term);
}
std::vector<std::string> splitComma(const std::string& input) {
    if (input.empty() || input.front() == ',' || input.back() == ',') {
        throw ParseException("Malformed comma-separated list");
    }
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start < input.size()) {
        const std::size_t end = input.find(',', start);
        const std::string part = input.substr(start,
            end == std::string::npos ? std::string::npos : end - start);
        if (part.empty()) throw ParseException("Empty list item");
        result.push_back(part);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return result;
}
CourseType parseType(const std::string& value) {
    if (value == "REGULAR") return CourseType::Regular;
    if (value == "SU") return CourseType::Su;
    if (value == "THESIS") return CourseType::Thesis;
    throw ParseException("Invalid course type: " + value);
}
Category parseCategory(const std::string& value) {
    if (value == "CORE") return Category::Core;
    if (value == "ELECTIVE") return Category::Elective;
    if (value == "MANDATORY_SU") return Category::MandatorySu;
    if (value == "OPEN") return Category::Open;
    throw ParseException("Invalid course category: " + value);
}
} // namespace

void Registrar::configure(const std::vector<std::string>& args) {
    JournalGuard journal(journal_, eventLine("PROGRAM", args));
    if (!courses_.empty()) throw ParseException("PROGRAM must appear before any COURSE");
    ProgramRules next;
    std::set<std::string> seen;
    for (const std::string& argument : args) {
        const std::size_t equal = argument.find('=');
        if (equal == std::string::npos || equal == 0 || equal + 1 == argument.size()) {
            throw ParseException("PROGRAM needs key=value tokens");
        }
        const std::string key = argument.substr(0, equal);
        const std::string value = argument.substr(equal + 1);
        if (!seen.insert(key).second) throw ParseException("Repeated PROGRAM key: " + key);
        if (key == "total") next.total = parseUnsigned(value);
        else if (key == "elective") next.elective = parseUnsigned(value);
        else if (key == "mingpa") next.minGpa100 = parseHundredths(value);
        else if (key == "thesis_min_grade") {
            try {
                const Grade g = Grade::create(value);
                if (!g.isLetter() || !g.isPass()) {
                    throw ParseException("Invalid minimum thesis letter");
                }
            } catch (const InvalidGradeSymbolException&) {
                throw ParseException("Invalid minimum thesis letter");
            }
            next.thesisMinGrade = value;
        }
        else if (key == "thesis_min_credits") next.thesisMinCredits = parseUnsigned(value);
        else if (key == "thesis_min_gpa") next.thesisMinGpa100 = parseHundredths(value);
        else if (key == "maxload") next.maxLoad = parseUnsigned(value);
        else throw ParseException("Unknown PROGRAM key: " + key);
    }
    if (seen.size() != 7) throw ParseException("PROGRAM requires all seven keys");
    std::swap(rules_, next); // Commit only after all seven fields are validated.
    journal.commit();
}

void Registrar::addCourse(const std::vector<std::string>& args) {
    JournalGuard journal(journal_, eventLine("COURSE", args));
    const std::string& code = args[0];
    const CourseType type = parseType(args[1]);
    const int credits = parseUnsigned(args[2]);
    const Category category = parseCategory(args[3]);
    if (code.empty() || courses_.count(code) != 0) {
        throw DuplicateEntityException("Duplicate course: " + code);
    }
    if (credits > 24 || (type != CourseType::Su && credits == 0) ||
        (type != CourseType::Su && category == Category::MandatorySu)) {
        throw InvalidCourseException("Invalid credits/category for course: " + code);
    }

    std::vector<Prerequisite> prerequisites;
    std::vector<std::string> antiRequisites;
    bool preSeen = false;
    bool antiSeen = false;
    for (std::size_t i = 4; i < args.size(); ++i) {
        const std::string& opt = args[i];
        if (opt.rfind("PRE=", 0) == 0 && !preSeen) {
            preSeen = true;
            for (const std::string& p : splitComma(opt.substr(4))) {
                const std::size_t colon = p.find(':');
                const std::string id = p.substr(0, colon);
                if (id == code || courses_.count(id) == 0) {
                    throw InvalidCourseException("Self/unknown prerequisite: " + id);
                }
                const Course& prerequisiteCourse = *courses_.at(id);
                std::string minimum = "D";
                if (colon != std::string::npos) minimum = p.substr(colon + 1);
                else if (prerequisiteCourse.type() == CourseType::Su) minimum = "S";
                try {
                    const Grade grade = Grade::create(minimum);
                    if (!grade.isPass() || grade.symbol() == "I") {
                        throw InvalidCourseException("Minimum prerequisite must be passing");
                    }
                    // Thesis prerequisites refer to final converted LETTER grades,
                    // not thesis progress grades (S/X).
                    if (prerequisiteCourse.type() == CourseType::Thesis) {
                        if (!grade.isLetter()) {
                            throw InvalidCourseException("Thesis prerequisite needs letter minimum");
                        }
                    } else {
                        prerequisiteCourse.validateGrade(grade);
                    }
                } catch (const InvalidGradeSymbolException&) {
                    throw InvalidCourseException("Invalid prerequisite minimum grade");
                } catch (const GradeTypeMismatchException&) {
                    throw InvalidCourseException("Prerequisite minimum incompatible with course");
                }
                prerequisites.push_back({id, minimum});
            }
        } else if (opt.rfind("ANTI=", 0) == 0 && !antiSeen) {
            antiSeen = true;
            antiRequisites = splitComma(opt.substr(5));
            for (const std::string& anti : antiRequisites) {
                if (anti == code) throw InvalidCourseException("Course cannot be anti-requisite of itself");
            }
        } else throw ParseException("Unknown/repeated COURSE option: " + opt);
    }

    std::unique_ptr<Course> course;
    if (type == CourseType::Regular) {
        course = std::make_unique<RegularCourse>(code, credits, category,
                                                  prerequisites, antiRequisites);
    } else if (type == CourseType::Su) {
        course = std::make_unique<SuCourse>(code, credits, category,
                                             prerequisites, antiRequisites);
    } else {
        course = std::make_unique<ThesisCourse>(code, credits, category,
                                                 prerequisites, antiRequisites);
    }
    courses_.emplace(code, std::move(course));
    journal.commit();
}

void Registrar::addStudent(const std::string& roll, const std::string& name) {
    JournalGuard journal(journal_, eventLine("STUDENT", {roll, name}));
    if (students_.count(roll) != 0) throw DuplicateEntityException("Duplicate student: " + roll);
    StudentHandle student(allocator_->create(roll, name), StudentPoolDeleter{allocator_.get()});
    students_.emplace(roll, std::move(student));
    journal.commit();
}

const Student& Registrar::requireStudent(const std::string& roll) const {
    auto it = students_.find(roll);
    if (it == students_.end()) throw UnknownStudentException("Unknown student: " + roll);
    return *it->second;
}
Student& Registrar::requireStudent(const std::string& roll) {
    return const_cast<Student&>(static_cast<const Registrar&>(*this).requireStudent(roll));
}
const Course& Registrar::requireCourse(const std::string& code) const {
    auto it = courses_.find(code);
    if (it == courses_.end()) throw UnknownCourseException("Unknown course: " + code);
    return *it->second;
}

bool Registrar::passedAtLeast(const Student& student, const Course& course,
                              const std::string& minimum) const {
    const Attempt* attempt = student.countingAttempt(course.code());
    if (attempt == nullptr || attempt->grade() == nullptr) return false;
    const Grade& grade = *attempt->grade();
    if (!course.earnsCredits(grade)) return false;
    if (course.type() == CourseType::Su) return grade.symbol() == "S";
    // Compare *grade points*, not grade-symbol strings.
    return grade.isLetter() && grade.points() >= Grade::create(minimum).points();
}

int Registrar::earnedCredits(const Student& student) const {
    int total = 0;
    for (const auto& [code, course] : courses_) {
        const Attempt* attempt = student.countingAttempt(code);
        if (attempt != nullptr && attempt->grade() != nullptr &&
            course->earnsCredits(*attempt->grade())) {
            total += course->credits();
        }
    }
    return total;
}

std::optional<EligibilityIssue> Registrar::eligibilityIssue(
        const std::string& roll, const std::string& courseCode,
        const std::string& term) const {
    requireValidTerm(term);
    const Student& student = requireStudent(roll); // E1: student before course.
    const Course& course = requireCourse(courseCode);

    // E2: normally a passed course cannot be repeated. The one exception
    // is a not-yet-used improvement registration for a REGULAR C- or D.
    const Attempt* last = student.countingAttempt(courseCode);
    if (last != nullptr && last->grade() != nullptr &&
        course.earnsCredits(*last->grade())) {
        const std::string& symbol = last->grade()->symbol();
        const bool improve = course.type() == CourseType::Regular &&
            (symbol == "C-" || symbol == "D") &&
            !student.hasUsedImprovement(courseCode);
        if (!improve) return EligibilityIssue::AlreadyPassed;
    }

    // E3: an open registration (including I and open thesis) blocks a new
    // registration. A duplicate term is also invalid.
    if (student.hasOpenAttempt(courseCode) ||
        std::any_of(student.attempts().begin(), student.attempts().end(),
                    [&](const Attempt& a) {
                        return a.courseCode() == courseCode && a.term() == term;
                    })) {
        return EligibilityIssue::DuplicateReg;
    }

    // E4: all prerequisites must be passed by their *counting attempt*.
    for (const Prerequisite& pre : course.prerequisites()) {
        const Course& prerequisite = requireCourse(pre.courseCode);
        if (!passedAtLeast(student, prerequisite, pre.minimumGrade)) {
            return EligibilityIssue::Prereq;
        }
    }

    // E5: passed anti-requisites prevent registration. Forward references
    // in ANTI are allowed; a missing course is not a passed course.
    for (const std::string& anti : course.antiRequisites()) {
        const auto it = courses_.find(anti);
        if (it != courses_.end()) {
            const Attempt* counted = student.countingAttempt(anti);
            if (counted != nullptr && counted->grade() != nullptr &&
                it->second->earnsCredits(*counted->grade())) {
                return EligibilityIssue::Antireq;
            }
        }
    }

    // E6: registered load, not earned credits. Include completed courses
    // and repeats in the term; thesis only counts in its registration term.
    std::int64_t load = course.credits();
    for (const Attempt& a : student.attempts()) {
        if (a.term() == term) {
            load += requireCourse(a.courseCode()).credits();
        }
    }
    if (load > rules_.maxLoad) return EligibilityIssue::CreditLimit;

    // E7: a thesis requires sufficient earned credits and existing GPA,
    // and no other thesis may be open or passed.
    if (course.type() == CourseType::Thesis) {
        if (earnedCredits(student) < rules_.thesisMinCredits) {
            return EligibilityIssue::ThesisNotAllowed;
        }
        try {
            if (gpaHundredths(roll) < rules_.thesisMinGpa100) {
                return EligibilityIssue::ThesisNotAllowed;
            }
        } catch (const NoGpaDataException&) {
            return EligibilityIssue::ThesisNotAllowed;
        }
        for (const Attempt& a : student.attempts()) {
            if (!a.isThesis()) continue;
            if (a.isOpen() || (a.grade() != nullptr && a.grade()->isPass())) {
                return EligibilityIssue::ThesisNotAllowed;
            }
        }
    }
    return std::nullopt;
}

std::string Registrar::issueCode(EligibilityIssue issue) {
    switch (issue) {
        case EligibilityIssue::AlreadyPassed: return "E2";
        case EligibilityIssue::DuplicateReg: return "E3";
        case EligibilityIssue::Prereq: return "E4";
        case EligibilityIssue::Antireq: return "E5";
        case EligibilityIssue::CreditLimit: return "E6";
        case EligibilityIssue::ThesisNotAllowed: return "E7";
    }
    throw ParseException("Unknown eligibility result");
}

void Registrar::registerCourse(const std::string& roll, const std::string& courseCode,
                               const std::string& term) {
    JournalGuard journal(journal_, eventLine("REGISTER", {roll, courseCode, term}));
    const auto failure = eligibilityIssue(roll, courseCode, term);
    if (failure.has_value()) {
        switch (*failure) {
            case EligibilityIssue::AlreadyPassed:
                throw AlreadyPassedException("Course already passed (no improvement available)");
            case EligibilityIssue::DuplicateReg:
                throw DuplicateRegistrationException("Existing open/same-term registration");
            case EligibilityIssue::Prereq:
                throw PrerequisiteNotMetException("A prerequisite is not satisfied");
            case EligibilityIssue::Antireq:
                throw AntiRequisiteException("An anti-requisite has been passed");
            case EligibilityIssue::CreditLimit:
                throw CreditLimitExceededException("Term credit limit exceeded");
            case EligibilityIssue::ThesisNotAllowed:
                throw ThesisNotAllowedException("Thesis admission requirements not satisfied");
        }
    }
    Student& student = requireStudent(roll);
    const Course& course = requireCourse(courseCode);
    const Attempt* last = student.countingAttempt(courseCode);
    const bool improve = last != nullptr && last->grade() != nullptr &&
        course.type() == CourseType::Regular &&
        (last->grade()->symbol() == "C-" || last->grade()->symbol() == "D") &&
        !student.hasUsedImprovement(courseCode);
    // All validations precede mutation; addAttempt is strongly exception-safe.
    student.addAttempt(courseCode, term, course.type() == CourseType::Thesis, improve);
    journal.commit();
}

std::vector<std::string> Registrar::graduationFailures(const std::string& roll) const {
    const Student& student = requireStudent(roll);
    std::vector<std::string> failures;

    // R1: earned credits from counting attempts only.
    if (earnedCredits(student) < rules_.total) failures.push_back("R1");

    // R2, R3, R4: all catalogue cores, elective credits, mandatory SU.
    bool coreComplete = true;
    bool suComplete = true;
    int electiveCredits = 0;
    bool thesisComplete = false;
    const int minimumThesisPoints = Grade::create(rules_.thesisMinGrade).points();
    for (const auto& [code, course] : courses_) {
        const Attempt* attempt = student.countingAttempt(code);
        const Grade* grade = attempt == nullptr ? nullptr : attempt->grade();
        const bool passed = grade != nullptr && course->earnsCredits(*grade);
        if (course->category() == Category::Core && !passed) coreComplete = false;
        if (course->category() == Category::Elective && passed) {
            electiveCredits += course->credits();
        }
        if (course->category() == Category::MandatorySu &&
            (grade == nullptr || grade->symbol() != "S")) suComplete = false;
        if (course->type() == CourseType::Thesis && grade != nullptr &&
            grade->isLetter() && grade->points() >= minimumThesisPoints &&
            course->earnsCredits(*grade)) {
            thesisComplete = true;
        }
    }
    if (!coreComplete) failures.push_back("R2");
    if (electiveCredits < rules_.elective) failures.push_back("R3");
    if (!suComplete) failures.push_back("R4");
    if (!thesisComplete) failures.push_back("R5");

    // R6: missing GPA data is a failure, not an E_NO_GPA exception.
    bool gpaComplete = false;
    try {
        gpaComplete = gpaHundredths(roll) >= rules_.minGpa100;
    } catch (const NoGpaDataException&) {
        gpaComplete = false;
    }
    if (!gpaComplete) failures.push_back("R6");

    // R7: a pending registration or I grade in a REGULAR/SU course.
    bool pending = false;
    for (const Attempt& attempt : student.attempts()) {
        if (!attempt.isThesis() && attempt.isOpen()) {
            pending = true;
            break;
        }
    }
    if (pending) failures.push_back("R7");
    return failures;
}

void Registrar::submitGrade(const std::string& roll, const std::string& courseCode,
                            const std::string& term, const std::string& symbol) {
    JournalGuard journal(journal_, eventLine("GRADE", {roll, courseCode, term, symbol}));
    requireValidTerm(term);
    Student& student = requireStudent(roll);
    const Course& course = requireCourse(courseCode);
    Grade grade = Grade::create(symbol);
    course.validateGrade(grade);
    if (course.type() == CourseType::Thesis) {
        Attempt* attempt = student.latestThesisAttempt(courseCode);
        if (attempt == nullptr) throw NotRegisteredException("No thesis registration");
        attempt->submitThesisProgress(term, grade);
    } else {
        Attempt* attempt = student.findAttempt(courseCode, term);
        if (attempt == nullptr) throw NotRegisteredException("No matching registration");
        attempt->submitGrade(grade);
    }
    journal.commit();
}

void Registrar::convertThesis(const std::string& roll, const std::string& courseCode,
                              const std::string& letter) {
    JournalGuard journal(journal_, eventLine("CONVERT", {roll, courseCode, letter}));
    Student& student = requireStudent(roll);
    const Course& course = requireCourse(courseCode);
    const Grade grade = Grade::create(letter);
    if (course.type() != CourseType::Thesis || !grade.isLetter()) {
        throw GradeTypeMismatchException("CONVERT requires a thesis and letter grade");
    }
    Attempt* attempt = student.latestThesisAttempt(courseCode);
    if (attempt == nullptr) throw NotRegisteredException("No thesis registration");
    attempt->convertThesis(grade);
    journal.commit();
}

int Registrar::gpaHundredths(const std::string& roll) const {
    const Student& student = requireStudent(roll);
    std::int64_t numerator = 0;
    std::int64_t denominator = 0;
    for (const auto& item : courses_) {
        const Course& course = *item.second;
        const Attempt* attempt = student.countingAttempt(item.first);
        if (attempt == nullptr || attempt->grade() == nullptr) continue;
        const Grade& grade = *attempt->grade();
        if (!course.countsInGpa(grade)) continue;
        numerator += static_cast<std::int64_t>(course.credits()) * grade.points();
        denominator += course.credits();
    }
    if (denominator == 0) throw NoGpaDataException("No GPA-eligible grades available");
    // Exact rational half-up rounding: floor((200*n + d)/(2*d)).
    const std::int64_t scaled = (200 * numerator + denominator) / (2 * denominator);
    if (scaled > std::numeric_limits<int>::max()) throw NoGpaDataException("GPA overflow");
    return static_cast<int>(scaled);
}

std::string Registrar::formatHundredths(int scaled) {
    std::ostringstream output;
    output << (scaled / 100) << '.' << std::setw(2) << std::setfill('0') << (scaled % 100);
    return output.str();
}

std::string Registrar::averageGrade(const std::string& roll) const {
    const int rounded = gpaHundredths(roll);
    if (rounded >= 950) return "A";
    if (rounded >= 850) return "A-";
    if (rounded >= 750) return "B";
    if (rounded >= 650) return "B-";
    if (rounded >= 550) return "C";
    if (rounded >= 450) return "C-";
    if (rounded >= 350) return "D";
    if (rounded >= 100) return "E";
    return "F";
}

// One deterministic, single-line transcript (the PDF leaves this format open).
// The output is fully assembled before any bytes are written to stdout.
std::string Registrar::transcript(const std::string& roll) const {
    const Student& student = requireStudent(roll);
    std::ostringstream out;
    out << "TRANSCRIPT " << student.roll() << ' ' << student.name();
    const auto& attempts = student.attempts();
    if (attempts.empty()) return out.str() + " EMPTY";

    // Cache the latest completed attempt per course. Even a terminated thesis
    // is a counting attempt, although it has no final GPA-eligible letter.
    std::unordered_map<std::string, const Attempt*> counted;
    for (const Attempt& a : attempts) {
        if (!a.hasFinalResult()) continue;
        auto it = counted.find(a.courseCode());
        if (it == counted.end() || a.term() > it->second->term()) {
            counted[a.courseCode()] = &a;
        }
    }

    // Sorting by term, then course makes the transcript independent of the
    // order of REGISTER commands and preserves attempt history deterministically.
    std::vector<const Attempt*> sorted;
    sorted.reserve(attempts.size());
    for (const Attempt& a : attempts) sorted.push_back(&a);
    std::stable_sort(sorted.begin(), sorted.end(), [](const Attempt* lhs, const Attempt* rhs) {
        if (lhs->term() != rhs->term()) return lhs->term() < rhs->term();
        return lhs->courseCode() < rhs->courseCode();
    });

    for (const Attempt* a : sorted) {
        out << " | " << a->courseCode() << '@' << a->term() << " grade=";
        if (a->isThesis()) {
            const ThesisRecord& thesis = *a->thesis();
            out << "THESIS(";
            switch (thesis.state()) {
                case ThesisState::Registered: out << "REGISTERED"; break;
                case ThesisState::InProgress: out << "IN_PROGRESS"; break;
                case ThesisState::AtRisk: out << "AT_RISK"; break;
                case ThesisState::Terminated: out << "TERMINATED"; break;
                case ThesisState::Final: out << "FINAL:" << thesis.finalGrade()->symbol(); break;
            }
            out << ";PROGRESS=";
            if (thesis.history().empty()) {
                out << "NONE";
            } else {
                bool first = true;
                for (const ThesisProgress& entry : thesis.history()) {
                    if (!first) out << ',';
                    out << entry.term << ':' << entry.symbol;
                    first = false;
                }
            }
            out << ')';
        } else if (a->grade() != nullptr) {
            out << a->grade()->symbol();
        } else {
            out << "PENDING";
        }
        const auto it = counted.find(a->courseCode());
        out << " counting=" << (it != counted.end() && it->second == a ? "YES" : "NO");
        out << " improvement=" << (a->isImprovement() ? "YES" : "NO");
    }
    return out.str();
}

// -------------------- Optional bonus functions --------------------

std::string Registrar::eventLine(const std::string& command,
                                 const std::vector<std::string>& args) {
    std::string event = command;
    for (const std::string& arg : args) event += " " + arg;
    return event;
}

void Registrar::authorize(const std::string& token, const std::string& reason) {
    // "DEAN" is a classroom demonstration token, not real-world authentication.
    if (token != "DEAN") throw AuthorizationException("DEAN token required");
    if (reason.empty() || reason.find_first_not_of(" \t") == std::string::npos) {
        throw ParseException("A nonempty reason is required");
    }
}

std::string Registrar::audit(const std::string& roll) const {
    std::string result = "AUDIT " + roll;
    bool found = false;
    for (const AuditRecord& entry : auditLog_) {
        if (entry.roll != roll) continue;
        result += " | " + entry.description;
        found = true;
    }
    return found ? result : result + " EMPTY";
}

namespace {
class AuditGuard {
public:
    AuditGuard(std::vector<AuditRecord>& log, AuditRecord record) : log_(log) {
        log_.push_back(std::move(record));
    }
    AuditGuard(const AuditGuard&) = delete;
    AuditGuard& operator=(const AuditGuard&) = delete;
    ~AuditGuard() noexcept { if (!committed_) log_.pop_back(); }
    void commit() noexcept { committed_ = true; }
private:
    std::vector<AuditRecord>& log_;
    bool committed_ = false;
};

void appendAuditLine(const std::string& description) {
    std::ofstream out("audit.log", std::ios::app);
    if (!out) throw StorageException("Cannot open audit.log for appending");
    out << description << '\n';
    out.flush();
    if (!out) throw StorageException("Failed to append to audit.log");
}

// Compact corruption detection for the replayable file; deliberately not a
// cryptographic signature. It also catches accidental line edits/truncation.
std::uint64_t journalChecksum(const std::vector<std::string>& journal) noexcept {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (const std::string& event : journal) {
        for (unsigned char ch : event) {
            hash ^= ch;
            hash *= UINT64_C(1099511628211);
        }
        hash ^= static_cast<unsigned char>('\n');
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

void replaceFile(const std::string& temporary, const std::string& destination) {
    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
#ifdef _WIN32
    // Windows does not consistently rename over an existing destination.
    if (error) {
        std::filesystem::remove(destination, error);
        error.clear();
        std::filesystem::rename(temporary, destination, error);
    }
#endif
    if (error) {
        std::filesystem::remove(temporary);
        throw StorageException("Cannot replace file " + destination + ": " + error.message());
    }
}
} // namespace

void Registrar::changeGrade(const std::string& roll, const std::string& courseCode,
                            const std::string& term, const std::string& symbol,
                            const std::string& deanToken, const std::string& reason) {
    authorize(deanToken, reason);
    requireValidTerm(term);
    Student& student = requireStudent(roll);
    const Course& course = requireCourse(courseCode);
    Attempt* attempt = student.findAttempt(courseCode, term);
    if (attempt == nullptr) throw NotRegisteredException("No matching registration");
    if (!attempt->hasFinalResult() || attempt->grade() == nullptr) {
        throw ChangeNotAllowedException("Only a final letter/SU grade can be corrected");
    }

    const Grade replacement = Grade::create(symbol);
    if (course.type() == CourseType::Thesis) {
        if (!replacement.isLetter()) throw GradeTypeMismatchException("Thesis correction requires letter");
    } else {
        course.validateGrade(replacement);
        if (!replacement.isFinal()) throw ChangeNotAllowedException("Cannot change final grade to I");
    }
    if (replacement.symbol() == attempt->grade()->symbol()) {
        throw ChangeNotAllowedException("A grade change requires a different grade");
    }

    // Allocate ALL potentially throwing in-memory resources before changing
    // the final grade. Both staged records roll back on file I/O errors.
    auto prepared = std::make_shared<const Grade>(replacement);
    const std::string oldSymbol = attempt->grade()->symbol();
    const std::string description = "CHANGE " + roll + " " + courseCode + " " + term +
         " " + oldSymbol + "->" + symbol + " reason=" + reason;
    JournalGuard journal(journal_, eventLine("CHANGE",
                          {roll, courseCode, term, symbol, deanToken, reason}));
    AuditGuard auditGuard(auditLog_, {roll, description});
    if (!replaying_) appendAuditLine(description);

    attempt->replaceFinalGrade(std::move(prepared)); // noexcept commit
    auditGuard.commit();
    journal.commit();
}

void Registrar::removeStudent(const std::string& roll, const std::string& deanToken,
                              const std::string& reason) {
    authorize(deanToken, reason);
    const auto it = students_.find(roll);
    if (it == students_.end()) throw UnknownStudentException("Unknown student: " + roll);
    const std::string description = "REMOVE_STUDENT " + roll + " reason=" + reason;
    JournalGuard journal(journal_, eventLine("REMOVE_STUDENT", {roll, deanToken, reason}));
    AuditGuard auditGuard(auditLog_, {roll, description});
    if (!replaying_) appendAuditLine(description);

    students_.erase(it);                      // nonthrowing StudentHandle destruction
    allocator_->releaseUnusedBlocks();        // frees completely empty slabs
    auditGuard.commit();
    journal.commit();
}

void Registrar::save(const std::string& path) const {
    if (path.empty() || path == "audit.log") {
        throw StorageException("Invalid SAVE path");
    }
    const std::string temporary = path + ".acaderp_tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) throw StorageException("Cannot open SAVE destination: " + path);
        out << "ACADERP_JOURNAL_V1 " << journal_.size() << ' '
            << journalChecksum(journal_) << '\n';
        for (const std::string& event : journal_) out << event << '\n';
        out.flush();
        if (!out) throw StorageException("Cannot finish SAVE file: " + path);
    }
    replaceFile(temporary, path);
}

void Registrar::replayEvent(const std::string& event) {
    CommandParser parser;
    const auto command = parser.parse(event);
    if (!command) throw StorageException("Journal has a blank event");
    const auto& a = command->args;
    switch (command->kind) {
        case CommandKind::Program: configure(a); break;
        case CommandKind::Course: addCourse(a); break;
        case CommandKind::Student: addStudent(a[0], a[1]); break;
        case CommandKind::Register: registerCourse(a[0], a[1], a[2]); break;
        case CommandKind::Grade: submitGrade(a[0], a[1], a[2], a[3]); break;
        case CommandKind::Convert: convertThesis(a[0], a[1], a[2]); break;
        case CommandKind::Change:
            changeGrade(a[0], a[1], a[2], a[3], a[4], joinReason(a, 5));
            break;
        case CommandKind::RemoveStudent:
            removeStudent(a[0], a[1], joinReason(a, 2));
            break;
        default: throw StorageException("Non-mutating event in SAVE file");
    }
}

void Registrar::writeAuditLogSnapshot() const {
    const std::string temporary = "audit.log.acaderp_tmp";
    {
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) throw StorageException("Cannot rebuild audit.log");
        for (const AuditRecord& entry : auditLog_) out << entry.description << '\n';
        out.flush();
        if (!out) throw StorageException("Failed rebuilding audit.log");
    }
    replaceFile(temporary, "audit.log");
}

void Registrar::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw StorageException("Cannot open LOAD file: " + path);
    std::string header;
    if (!std::getline(in, header)) throw StorageException("Empty LOAD file");
    std::istringstream meta(header);
    std::string magic, extra;
    std::size_t count = 0;
    std::uint64_t hash = 0;
    if (!(meta >> magic >> count >> hash) || (meta >> extra) ||
        magic != "ACADERP_JOURNAL_V1" || count > 2000000) {
        throw StorageException("Invalid LOAD file header");
    }
    std::vector<std::string> events;
    events.reserve(count);
    std::string line;
    for (std::size_t i = 0; i < count; ++i) {
        if (!std::getline(in, line) || line.empty() || line.size() > 1048576) {
            throw StorageException("Truncated or invalid LOAD event");
        }
        events.push_back(line);
    }
    if (std::getline(in, line) || journalChecksum(events) != hash) {
        throw StorageException("LOAD file count/checksum mismatch");
    }

    Registrar restored;
    restored.replaying_ = true;
    try {
        for (const std::string& event : events) restored.replayEvent(event);
    } catch (const ErpException& error) {
        throw StorageException("Invalid saved academic history: " + std::string(error.what()));
    }
    restored.replaying_ = false;
    // Disk log is brought into line with the incoming snapshot before the
    // in-memory swap. No individual student is changed during validation.
    restored.writeAuditLogSnapshot();

    using std::swap;
    swap(rules_, restored.rules_);
    courses_.swap(restored.courses_);
    students_.swap(restored.students_);
    allocator_.swap(restored.allocator_);
    journal_.swap(restored.journal_);
    auditLog_.swap(restored.auditLog_);
}
