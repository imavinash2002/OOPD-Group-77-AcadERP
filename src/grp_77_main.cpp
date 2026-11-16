// group-77
#include "grp_77_CommandParser.h"
#include "grp_77_Exceptions.h"
#include "grp_77_Registrar.h"
#include <iostream>
#include <string>

int run() {
    Registrar registrar;
    CommandParser parser;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(std::cin, line)) {
        ++lineNumber;
        try {
            const auto command = parser.parse(line);
            if (!command.has_value()) continue;
            const auto& args = command->args;
            switch (command->kind) {
                case CommandKind::Save:
                    registrar.save(args[0]);
                    break;
                case CommandKind::Load:
                    registrar.load(args[0]);
                    break;
                case CommandKind::Change: {
                    std::string reason;
                    for (std::size_t i = 5; i < args.size(); ++i) {
                        if (!reason.empty()) reason += ' ';
                        reason += args[i];
                    }
                    registrar.changeGrade(args[0], args[1], args[2], args[3], args[4], reason);
                    break;
                }
                case CommandKind::RemoveStudent: {
                    std::string reason;
                    for (std::size_t i = 2; i < args.size(); ++i) {
                        if (!reason.empty()) reason += ' ';
                        reason += args[i];
                    }
                    registrar.removeStudent(args[0], args[1], reason);
                    break;
                }
                case CommandKind::Audit: {
                    const std::string report = registrar.audit(args[0]);
                    std::cout << report << '\n';
                    break;
                }
                case CommandKind::Program:
                    registrar.configure(args);
                    break;
                case CommandKind::Course:
                    registrar.addCourse(args);
                    break;
                case CommandKind::Student:
                    registrar.addStudent(args[0], args[1]);
                    break;
                case CommandKind::Register:
                    registrar.registerCourse(args[0], args[1], args[2]);
                    break;
                case CommandKind::Grade:
                    registrar.submitGrade(args[0], args[1], args[2], args[3]);
                    break;
                case CommandKind::Convert:
                    registrar.convertThesis(args[0], args[1], args[2]);
                    break;
                case CommandKind::Eligible: {
                    const auto failure = registrar.eligibilityIssue(args[0], args[1], args[2]);
                    std::cout << "ELIGIBLE " << args[0] << ' ' << args[1] << ' ';
                    if (failure.has_value())
                        std::cout << "NO " << Registrar::issueCode(*failure) << '\n';
                    else
                        std::cout << "YES\n";
                    break;
                }
                case CommandKind::Grad: {
                    const auto failures = registrar.graduationFailures(args[0]);
                    std::cout << "GRAD " << args[0];
                    if (failures.empty()) {
                        std::cout << " YES\n";
                    } else {
                        std::cout << " NO ";
                        for (std::size_t i = 0; i < failures.size(); ++i) {
                            if (i != 0) std::cout << ',';
                            std::cout << failures[i];
                        }
                        std::cout << '\n';
                    }
                    break;
                }
                case CommandKind::Avg: {
                    // Compute first: on failure, stdout must contain only ERROR.
                    const std::string average = registrar.averageGrade(args[0]);
                    std::cout << "AVG " << args[0] << ' ' << average << '\n';
                    break;
                }
                case CommandKind::Transcript: {
                    // Build full output before writing to prevent partial lines on errors.
                    const std::string result = registrar.transcript(args[0]);
                    std::cout << result << '\n';
                    break;
                }
                case CommandKind::Gpa: {
                    // Finish the potentially throwing computation before printing.
                    const int rounded = registrar.gpaHundredths(args[0]);
                    std::cout << "GPA " << args[0] << ' '
                              << Registrar::formatHundredths(rounded) << '\n';
                    break;
                }
            }
        } catch (const ErpException& error) {
            std::cout << "ERROR " << lineNumber << ' ' << error.code() << '\n';
            std::cerr << "Line " << lineNumber << ": " << error.what() << '\n';
        }
    }
    return 0;
}

int main() {
    try { return run(); }
    catch (...) { // Only permitted catch-all: outer unexpected-error guard.
        std::cerr << "FATAL\n";
        return 2;
    }
}
