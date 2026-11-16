// group-77
#include "grp_77_StudentAllocator.h"
#include <algorithm>
#include <new>
#include <utility>

StudentAllocator::~StudentAllocator() noexcept {
    // Registrar owns StudentHandles, which normally destroy objects first.
    // This also safely cleans up stand-alone pool use in a unit test.
    for (const auto& block : blocks_) {
        for (std::size_t i = 0; i < blockSize; ++i) {
            if (block->occupied[i]) {
                std::launder(reinterpret_cast<Student*>(&block->slots[i]))->~Student();
            }
        }
    }
}

Student* StudentAllocator::create(const std::string& roll, const std::string& name) {
    Block* selected = nullptr;
    for (auto& block : blocks_) {
        if (block->used != blockSize) { selected = block.get(); break; }
    }
    if (selected == nullptr) {
        auto newBlock = std::make_unique<Block>();
        blocks_.push_back(std::move(newBlock));
        selected = blocks_.back().get();
    }
    for (std::size_t i = 0; i < blockSize; ++i) {
        if (!selected->occupied[i]) {
            // If construction throws, the slot remains free.
            Student* student = new (&selected->slots[i]) Student(roll, name);
            selected->occupied[i] = true;
            ++selected->used;
            return student;
        }
    }
    return nullptr; // unreachable when used < blockSize
}

void StudentAllocator::destroy(Student* student) noexcept {
    if (student == nullptr) return;
    for (const auto& block : blocks_) {
        for (std::size_t i = 0; i < blockSize; ++i) {
            if (block->occupied[i] &&
                reinterpret_cast<const void*>(&block->slots[i]) == static_cast<const void*>(student)) {
                student->~Student();
                block->occupied[i] = false;
                --block->used;
                return;
            }
        }
    }
}

void StudentAllocator::releaseUnusedBlocks() noexcept {
    blocks_.erase(std::remove_if(blocks_.begin(), blocks_.end(),
                 [](const std::unique_ptr<Block>& block) { return block->used == 0; }),
                 blocks_.end());
}

std::size_t StudentAllocator::liveCount() const noexcept {
    std::size_t total = 0;
    for (const auto& block : blocks_) total += block->used;
    return total;
}
