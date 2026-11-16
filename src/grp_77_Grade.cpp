// group-77
#include "grp_77_Grade.h"
#include "grp_77_Exceptions.h"

Grade Grade::create(const std::string& s) {
    if (s == "A")  return Grade(s, 10, true);
    if (s == "A-") return Grade(s, 9, true);
    if (s == "B")  return Grade(s, 8, true);
    if (s == "B-") return Grade(s, 7, true);
    if (s == "C")  return Grade(s, 6, true);
    if (s == "C-") return Grade(s, 5, true);
    if (s == "D")  return Grade(s, 4, true);
    if (s == "E")  return Grade(s, 2, false);
    if (s == "F")  return Grade(s, 0, false);
    if (s == "S")  return Grade(s, -1, true);
    if (s == "X")  return Grade(s, -1, false);
    if (s == "I")  return Grade(s, -1, false);
    throw InvalidGradeSymbolException("Unrecognized grade symbol: " + s);
}
