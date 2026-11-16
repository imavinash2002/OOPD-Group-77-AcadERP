// group-77
#include "grp_77_CommandParser.h"
#include "grp_77_Exceptions.h"
#include <sstream>

std::optional<Command> CommandParser::parse(const std::string& line) const {
    std::istringstream input(line);
    std::string verb;
    if (!(input >> verb) || verb[0] == '#') return std::nullopt;
    std::vector<std::string> args;
    std::string token;
    while (input >> token) args.push_back(token);

    CommandKind kind;
    if (verb == "PROGRAM") kind = CommandKind::Program;
    else if (verb == "COURSE") kind = CommandKind::Course;
    else if (verb == "STUDENT") kind = CommandKind::Student;
    else if (verb == "REGISTER") kind = CommandKind::Register;
    else if (verb == "GRADE") kind = CommandKind::Grade;
    else if (verb == "CONVERT") kind = CommandKind::Convert;
    else if (verb == "GPA") kind = CommandKind::Gpa;
    else if (verb == "AVG") kind = CommandKind::Avg;
    else if (verb == "ELIGIBLE") kind = CommandKind::Eligible;
    else if (verb == "GRAD") kind = CommandKind::Grad;
    else if (verb == "TRANSCRIPT") kind = CommandKind::Transcript;
    else if (verb == "SAVE") kind = CommandKind::Save;
    else if (verb == "LOAD") kind = CommandKind::Load;
    else if (verb == "CHANGE") kind = CommandKind::Change;
    else if (verb == "AUDIT") kind = CommandKind::Audit;
    else if (verb == "REMOVE_STUDENT") kind = CommandKind::RemoveStudent;
    else throw ParseException("Unknown/unimplemented command: " + verb);

    bool valid = false;
    switch (kind) {
        case CommandKind::Program: valid = args.size() == 7; break;
        case CommandKind::Course: valid = args.size() >= 4 && args.size() <= 6; break;
        case CommandKind::Student: valid = args.size() == 2; break;
        case CommandKind::Register: valid = args.size() == 3; break;
        case CommandKind::Grade: valid = args.size() == 4; break;
        case CommandKind::Convert: valid = args.size() == 3; break;
        case CommandKind::Gpa: valid = args.size() == 1; break;
        case CommandKind::Avg: valid = args.size() == 1; break;
        case CommandKind::Eligible: valid = args.size() == 3; break;
        case CommandKind::Grad: valid = args.size() == 1; break;
        case CommandKind::Transcript: valid = args.size() == 1; break;
        case CommandKind::Save: valid = args.size() == 1; break;
        case CommandKind::Load: valid = args.size() == 1; break;
        case CommandKind::Change: valid = args.size() >= 6; break;
        case CommandKind::Audit: valid = args.size() == 1; break;
        case CommandKind::RemoveStudent: valid = args.size() >= 3; break;
    }
    if (!valid) throw ParseException("Wrong number of tokens for " + verb);
    return Command{kind, std::move(args)};
}
