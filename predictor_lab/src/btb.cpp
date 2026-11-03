#include "btb.h"

BranchTargetBuffer::BranchTargetBuffer() : entries{} {}

std::size_t BranchTargetBuffer::getIndex(std::uint32_t branchPC) const {
    return static_cast<std::size_t>((branchPC >> 2) % TABLE_SIZE);
}

bool BranchTargetBuffer::lookup(
    std::uint32_t branchPC,
    std::uint32_t& target
) const {
    const std::size_t index = getIndex(branchPC);
    const Entry& entry = entries[index];

    if (!entry.valid || entry.tag != branchPC) {
        return false;
    }

    target = entry.target;
    return true;
}

void BranchTargetBuffer::update(
    std::uint32_t branchPC,
    std::uint32_t actualTarget
) {
    const std::size_t index = getIndex(branchPC);
    Entry& entry = entries[index];

    // Direct-mapped replacement: a conflicting PC overwrites this entry.
    entry.valid = true;
    entry.tag = branchPC;
    entry.target = actualTarget;
}
