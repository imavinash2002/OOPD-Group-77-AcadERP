// group-77
#include "grp_77_StudentAllocator.h"
#include "grp_77_Registrar.h"
#include "grp_77_Exceptions.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename E, typename Operation>
void expects(Operation operation) {
    try { operation(); }
    catch (const E&) { return; }
    throw std::runtime_error("Expected bonus exception was not raised");
}

void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

int main() {
    try {
        // 64-object slabs grow on demand, reuse empty slots, then shrink again.
        StudentAllocator pool;
        std::vector<Student*> addresses;
        for (int i = 0; i < 3200; ++i)
            addresses.push_back(pool.create("ID" + std::to_string(i), "Student"));
        check(pool.liveCount() == 3200, "Pool should hold 3200 live students");
        check(pool.blockCount() == 50, "3200 students should occupy 50 slabs");
        check(pool.capacity() == 3200, "Incorrect slab capacity");

        Student* original = addresses.front();
        pool.destroy(original);
        Student* reused = pool.create("REUSE", "Other");
        check(reused == original && reused->roll() == "REUSE", "Free slot was not reused");
        addresses.front() = reused;
        for (Student* student : addresses) pool.destroy(student);
        pool.releaseUnusedBlocks();
        check(pool.liveCount() == 0 && pool.capacity() == 0, "Pool did not shrink to zero");

        // Registrars actually use the pool, not merely a demonstration class.
        Registrar r;
        r.addCourse({"C", "REGULAR", "4", "OPEN"});
        for (int i = 0; i < 140; ++i)
            r.addStudent("S" + std::to_string(i), "Student");
        check(r.studentCount() == 140 && r.studentCapacity() == 192,
              "Registrar did not grow its allocator");
        for (int i = 10; i < 140; ++i)
            r.removeStudent("S" + std::to_string(i), "DEAN", "cohort_left");
        check(r.studentCount() == 10 && r.studentCapacity() == 64,
              "Registrar did not return unused slabs");
        check(r.audit("S10").find("cohort_left") != std::string::npos,
              "Removal audit entry missing");

        r.registerCourse("S0", "C", "2026-1");
        r.submitGrade("S0", "C", "2026-1", "B");
        r.changeGrade("S0", "C", "2026-1", "A", "DEAN", "clerical_error");
        const std::string before = r.transcript("S0");
        r.save("tests/bonus_unit.save");
        r.changeGrade("S0", "C", "2026-1", "F", "DEAN", "temporary_revision");
        check(r.gpaHundredths("S0") == 0, "Grade correction must affect GPA");
        r.load("tests/bonus_unit.save");
        check(r.gpaHundredths("S0") == 1000 && r.transcript("S0") == before,
              "Round-trip failed to restore correction");
        check(r.studentCount() == 10 && r.studentCapacity() == 64,
              "Round-trip failed to restore allocator population");
        check(r.audit("S0").find("clerical_error") != std::string::npos &&
              r.audit("S0").find("temporary_revision") == std::string::npos,
              "Round-trip failed to restore the audit journal");
        expects<AuthorizationException>([&] {
            r.changeGrade("S0", "C", "2026-1", "B", "WRONG", "unauthorized");
        });
        check(r.transcript("S0") == before, "Unauthorized correction changed state");
        expects<StorageException>([&] { r.load("tests/no_such_file.save"); });
        check(r.transcript("S0") == before, "Failed LOAD changed state");

        std::cout << "PASS: 3200-student allocator growth/shrink, SAVE/LOAD, audit, authorization\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
