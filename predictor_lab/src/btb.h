#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

class BranchTargetBuffer {

private:

    static constexpr std::size_t TABLE_SIZE = 16;

    struct Entry {
        bool valid = false;
        uint32_t tag = 0;
        uint32_t target = 0;
    };

    std::array<Entry, TABLE_SIZE> entries;

    std::size_t getIndex(uint32_t branchPC) const;

public:

    BranchTargetBuffer();

    bool lookup(uint32_t branchPC, uint32_t& target) const;

    void update(uint32_t branchPC, uint32_t actualTarget);
};