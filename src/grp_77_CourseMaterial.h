// group-77
#pragma once

#include <memory>
#include <string>
#include <vector>

// Independent cloned copies can be customized by different instructors.
class CourseMaterial {
public:
    virtual ~CourseMaterial() = default;
    virtual std::unique_ptr<CourseMaterial> clone() const = 0;
};

class StandardCourseMaterial final : public CourseMaterial {
public:
    std::string syllabus;
    std::vector<std::string> lectureNotes;
    std::vector<std::string> assignments;
    std::unique_ptr<CourseMaterial> clone() const override {
        return std::make_unique<StandardCourseMaterial>(*this);
    }
};
