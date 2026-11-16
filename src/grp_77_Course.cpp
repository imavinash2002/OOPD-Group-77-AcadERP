// group-77
#include "grp_77_Course.h"
#include "grp_77_Exceptions.h"
#include <utility>

Course::Course(std::string code, int credits, Category category,
               std::vector<Prerequisite> prerequisites,
               std::vector<std::string> antiRequisites)
    : code_(std::move(code)), credits_(credits), category_(category),
      prerequisites_(std::move(prerequisites)),
      antiRequisites_(std::move(antiRequisites)),
      materials_(std::make_unique<StandardCourseMaterial>()) {}

void Course::setMaterials(const CourseMaterial& material) {
    // Clone before committing to preserve the original on allocation failure.
    std::unique_ptr<CourseMaterial> copy = material.clone();
    materials_.swap(copy);
}

std::unique_ptr<CourseMaterial> Course::copyMaterialsForInstructor() const {
    return materials_->clone();
}

void RegularCourse::validateGrade(const Grade& grade) const {
    if (!grade.isLetter() && grade.symbol() != "I") {
        throw GradeTypeMismatchException("Regular courses need a letter or I");
    }
}
bool RegularCourse::countsInGpa(const Grade& grade) const noexcept {
    return grade.isLetter();
}
bool RegularCourse::earnsCredits(const Grade& grade) const noexcept {
    return grade.isLetter() && grade.isPass();
}

void SuCourse::validateGrade(const Grade& grade) const {
    if (grade.symbol() != "S" && grade.symbol() != "X" && grade.symbol() != "I") {
        throw GradeTypeMismatchException("SU courses need S, X or I");
    }
}
bool SuCourse::countsInGpa(const Grade&) const noexcept { return false; }
bool SuCourse::earnsCredits(const Grade& grade) const noexcept { return grade.symbol() == "S"; }

void ThesisCourse::validateGrade(const Grade& grade) const {
    if (grade.symbol() != "S" && grade.symbol() != "X") {
        throw GradeTypeMismatchException("Thesis progress grade needs S or X");
    }
}
bool ThesisCourse::countsInGpa(const Grade& grade) const noexcept {
    return grade.isLetter(); // Used after CONVERT is implemented in milestone 2.
}
bool ThesisCourse::earnsCredits(const Grade& grade) const noexcept {
    return grade.isLetter() && grade.isPass();
}
