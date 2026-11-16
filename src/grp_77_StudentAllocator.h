// group-77
#pragma once

#include "grp_77_Student.h"
#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

// Slab allocator: students have stable addresses, reused slots and reclaimable
// empty slabs. In particular it does not preallocate millions of Students.
class StudentAllocator {
public:
    static constexpr std::size_t blockSize = 64;
    StudentAllocator() = default;
    ~StudentAllocator() noexcept;
    StudentAllocator(const StudentAllocator&) = delete;
    StudentAllocator& operator=(const StudentAllocator&) = delete;

    Student* create(const std::string& roll, const std::string& name);
    void destroy(Student* student) noexcept;
    void releaseUnusedBlocks() noexcept;
    std::size_t liveCount() const noexcept;
    std::size_t blockCount() const noexcept { return blocks_.size(); }
    std::size_t capacity() const noexcept { return blocks_.size() * blockSize; }

private:
    using Slot = std::aligned_storage_t<sizeof(Student), alignof(Student)>;
    struct Block {
        std::array<Slot, blockSize> slots;
        std::array<bool, blockSize> occupied{};
        std::size_t used = 0;
    };
    std::vector<std::unique_ptr<Block>> blocks_;
};

// The pool outlives every StudentHandle (see Registrar's member order).
struct StudentPoolDeleter {
    StudentAllocator* allocator = nullptr;
    void operator()(Student* student) const noexcept {
        if (allocator != nullptr && student != nullptr) allocator->destroy(student);
    }
};
using StudentHandle = std::unique_ptr<Student, StudentPoolDeleter>;
