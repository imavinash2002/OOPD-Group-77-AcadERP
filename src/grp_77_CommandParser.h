// group-77
#pragma once

#include <optional>
#include <string>
#include <vector>

enum class CommandKind { Program, Course, Student, Register, Grade, Convert, Gpa, Avg, Eligible, Grad, Transcript, Save, Load, Change, Audit, RemoveStudent };
struct Command {
    CommandKind kind;
    std::vector<std::string> args;
};

class CommandParser {
public:
    std::optional<Command> parse(const std::string& line) const;
};
