// group-77
#pragma once

#include <stdexcept>
#include <string>

class ErpException : public std::runtime_error {
public:
    ErpException(std::string code, const std::string& message)
        : std::runtime_error(message), code_(std::move(code)) {}
    const std::string& code() const noexcept { return code_; }
private:
    std::string code_;
};

class InputException : public ErpException { public: using ErpException::ErpException; };
class EntityException : public ErpException { public: using ErpException::ErpException; };
class GradeException : public ErpException { public: using ErpException::ErpException; };
class RegistrationException : public ErpException { public: using ErpException::ErpException; };
class ComputationException : public ErpException { public: using ErpException::ErpException; };

#define DEFINE_ERP_ERROR(Name, Base, Code) \
class Name : public Base { \
public: \
    explicit Name(const std::string& message) : Base(Code, message) {} \
};

DEFINE_ERP_ERROR(ParseException, InputException, "E_PARSE")
DEFINE_ERP_ERROR(DuplicateEntityException, EntityException, "E_DUPLICATE")
DEFINE_ERP_ERROR(UnknownStudentException, EntityException, "E_UNKNOWN_STUDENT")
DEFINE_ERP_ERROR(UnknownCourseException, EntityException, "E_UNKNOWN_COURSE")
DEFINE_ERP_ERROR(InvalidCourseException, EntityException, "E_COURSE_DEF")
DEFINE_ERP_ERROR(InvalidGradeSymbolException, GradeException, "E_GRADE_SYMBOL")
DEFINE_ERP_ERROR(GradeTypeMismatchException, GradeException, "E_GRADE_TYPE")
DEFINE_ERP_ERROR(NotRegisteredException, GradeException, "E_NOT_REGISTERED")
DEFINE_ERP_ERROR(GradeAlreadyFinalException, GradeException, "E_GRADE_FINAL")
DEFINE_ERP_ERROR(InvalidThesisTransitionException, GradeException, "E_THESIS_TRANSITION")
DEFINE_ERP_ERROR(ThesisTerminatedException, GradeException, "E_THESIS_TERMINATED")
DEFINE_ERP_ERROR(AlreadyPassedException, RegistrationException, "E_ALREADY_PASSED")
DEFINE_ERP_ERROR(DuplicateRegistrationException, RegistrationException, "E_DUP_REG")
DEFINE_ERP_ERROR(PrerequisiteNotMetException, RegistrationException, "E_PREREQ")
DEFINE_ERP_ERROR(AntiRequisiteException, RegistrationException, "E_ANTIREQ")
DEFINE_ERP_ERROR(CreditLimitExceededException, RegistrationException, "E_CREDIT_LIMIT")
DEFINE_ERP_ERROR(ThesisNotAllowedException, RegistrationException, "E_THESIS_NOT_ALLOWED")
DEFINE_ERP_ERROR(NoGpaDataException, ComputationException, "E_NO_GPA")
// Bonus errors; core assignment codes remain unchanged.
DEFINE_ERP_ERROR(StorageException, InputException, "E_STORAGE")
DEFINE_ERP_ERROR(AuthorizationException, InputException, "E_AUTH")
DEFINE_ERP_ERROR(ChangeNotAllowedException, GradeException, "E_CHANGE_STATE")

#undef DEFINE_ERP_ERROR
