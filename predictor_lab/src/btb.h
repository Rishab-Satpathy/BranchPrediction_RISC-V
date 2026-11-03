#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class BranchTargetBuffer {
private:
    static constexpr std::size_t TABLE_SIZE = 16;

    struct Entry {
        bool valid = false;
        std::uint32_t tag = 0;
        std::uint32_t target = 0;
    };

    std::array<Entry, TABLE_SIZE> entries{};
    std::size_t getIndex(std::uint32_t branchPC) const;

public:
    BranchTargetBuffer();

    // Returns true on a valid tag match and writes the stored target to target.
    bool lookup(std::uint32_t branchPC, std::uint32_t& target) const;

    // Installs/replaces the entry selected by branchPC.
    void update(std::uint32_t branchPC, std::uint32_t actualTarget);
};
