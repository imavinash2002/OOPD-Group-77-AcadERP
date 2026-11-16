// group-77
#include "grp_77_Course.h"
#include "grp_77_CourseMaterial.h"
#include "grp_77_Exceptions.h"
#include "grp_77_Registrar.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename Exception, typename Operation>
void expectException(Operation operation) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("Expected exception not raised");
}

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        Registrar registrar;
        registrar.addCourse({"C1", "REGULAR", "4", "CORE"});
        registrar.addStudent("R", "Ray");

        // Rejected registration: no attempt must be inserted.
        const std::string empty = registrar.transcript("R");
        expectException<UnknownCourseException>([&]() {
            registrar.registerCourse("R", "MISSING", "2026-1");
        });
        require(registrar.transcript("R") == empty,
                "Unknown-course registration changed the student");

        // Rejected course definition must not insert a partial course.
        expectException<InvalidCourseException>([&]() {
            registrar.addCourse({"C2", "REGULAR", "4", "CORE", "PRE=MISSING"});
        });
        expectException<UnknownCourseException>([&]() {
            static_cast<void>(registrar.eligibilityIssue("R", "C2", "2026-1"));
        });

        registrar.registerCourse("R", "C1", "2026-1");
        const std::string pending = registrar.transcript("R");
        expectException<InvalidGradeSymbolException>([&]() {
            registrar.submitGrade("R", "C1", "2026-1", "Z");
        });
        expectException<GradeTypeMismatchException>([&]() {
            registrar.submitGrade("R", "C1", "2026-1", "S");
        });
        require(registrar.transcript("R") == pending,
                "Invalid grade changed the student");

        registrar.submitGrade("R", "C1", "2026-1", "D");
        const std::string final = registrar.transcript("R");
        expectException<GradeAlreadyFinalException>([&]() {
            registrar.submitGrade("R", "C1", "2026-1", "A");
        });
        require(registrar.transcript("R") == final,
                "Regrading changed a final grade");
        require(registrar.gpaHundredths("R") == 400,
                "Failed grade command changed GPA");

        // Once a C-/D improvement is registered, its open attempt must remain
        // unchanged if a second registration is rejected by eligibility E2.
        registrar.registerCourse("R", "C1", "2026-2");
        const std::string improvementPending = registrar.transcript("R");
        expectException<AlreadyPassedException>([&]() {
            registrar.registerCourse("R", "C1", "2026-3");
        });
        require(registrar.transcript("R") == improvementPending,
                "Rejected duplicate changed attempts");

        // Rejecting thesis conversion before progress must not change state.
        Registrar thesisRegistrar;
        thesisRegistrar.configure({"total=4", "elective=0", "mingpa=0.00",
                                  "thesis_min_grade=D", "thesis_min_credits=4",
                                  "thesis_min_gpa=0.00", "maxload=24"});
        thesisRegistrar.addCourse({"BASE", "REGULAR", "4", "OPEN"});
        thesisRegistrar.addCourse({"TH", "THESIS", "8", "OPEN"});
        thesisRegistrar.addStudent("T", "Tina");
        thesisRegistrar.registerCourse("T", "BASE", "2026-1");
        thesisRegistrar.submitGrade("T", "BASE", "2026-1", "A");
        thesisRegistrar.registerCourse("T", "TH", "2026-2");
        const std::string thesisBefore = thesisRegistrar.transcript("T");
        expectException<InvalidThesisTransitionException>([&]() {
            thesisRegistrar.convertThesis("T", "TH", "A");
        });
        require(thesisRegistrar.transcript("T") == thesisBefore,
                "Invalid conversion changed thesis");
        thesisRegistrar.submitGrade("T", "TH", "2026-2", "S");
        const std::string progressBefore = thesisRegistrar.transcript("T");
        expectException<InvalidThesisTransitionException>([&]() {
            thesisRegistrar.submitGrade("T", "TH", "2026-1", "X");
        });
        require(thesisRegistrar.transcript("T") == progressBefore,
                "Out-of-order progress changed thesis");

        // Clone produces an independent material object, including its vectors.
        RegularCourse course("C", 4, Category::Open, {}, {});
        StandardCourseMaterial templateMaterial;
        templateMaterial.syllabus = "Original";
        templateMaterial.lectureNotes.push_back("Lecture 1");
        templateMaterial.assignments.push_back("Assignment 1");
        course.setMaterials(templateMaterial);
        auto instructorOne = course.copyMaterialsForInstructor();
        auto instructorTwo = course.copyMaterialsForInstructor();
        auto* first = dynamic_cast<StandardCourseMaterial*>(instructorOne.get());
        auto* second = dynamic_cast<StandardCourseMaterial*>(instructorTwo.get());
        require(first != nullptr && second != nullptr, "Material clone has wrong type");
        first->syllabus = "Changed";
        first->lectureNotes.push_back("Lecture 2");
        first->assignments[0] = "Changed Assignment";
        require(second->syllabus == "Original" && second->lectureNotes.size() == 1 &&
                second->assignments[0] == "Assignment 1",
                "Second instructor's materials were mutated");
        require(templateMaterial.syllabus == "Original", "Template material was mutated");

        std::cout << "PASS: exception safety and independent material clones\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
