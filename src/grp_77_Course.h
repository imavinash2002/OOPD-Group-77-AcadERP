// group-77
#pragma once

#include "grp_77_Grade.h"
#include "grp_77_CourseMaterial.h"
#include <memory>
#include <string>
#include <vector>

enum class CourseType { Regular, Su, Thesis };
enum class Category { Core, Elective, MandatorySu, Open };

struct Prerequisite {
    std::string courseCode;
    std::string minimumGrade;
};

class Course {
public:
    Course(std::string code, int credits, Category category,
           std::vector<Prerequisite> prerequisites,
           std::vector<std::string> antiRequisites);
    virtual ~Course() = default;
    const std::string& code() const noexcept { return code_; }
    int credits() const noexcept { return credits_; }
    Category category() const noexcept { return category_; }
    const std::vector<Prerequisite>& prerequisites() const noexcept { return prerequisites_; }
    const std::vector<std::string>& antiRequisites() const noexcept { return antiRequisites_; }

    // Each course owns one teaching-material template. Instructors get an
    // independent virtual deep clone and can edit it without altering the course.
    void setMaterials(const CourseMaterial& material);
    std::unique_ptr<CourseMaterial> copyMaterialsForInstructor() const;

    virtual CourseType type() const noexcept = 0;
    virtual void validateGrade(const Grade& grade) const = 0;
    virtual bool countsInGpa(const Grade& grade) const noexcept = 0;
    virtual bool earnsCredits(const Grade& grade) const noexcept = 0;

private:
    std::string code_;
    int credits_;
    Category category_;
    std::vector<Prerequisite> prerequisites_;
    std::vector<std::string> antiRequisites_;
    std::unique_ptr<CourseMaterial> materials_;
};

class RegularCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Regular; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade& grade) const noexcept override;
    bool earnsCredits(const Grade& grade) const noexcept override;
};

class SuCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Su; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade& grade) const noexcept override;
    bool earnsCredits(const Grade& grade) const noexcept override;
};

class ThesisCourse final : public Course {
public:
    using Course::Course;
    CourseType type() const noexcept override { return CourseType::Thesis; }
    void validateGrade(const Grade& grade) const override;
    bool countsInGpa(const Grade& grade) const noexcept override;
    bool earnsCredits(const Grade& grade) const noexcept override;
};
