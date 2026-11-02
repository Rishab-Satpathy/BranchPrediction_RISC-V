#include "btb.h"

BranchTargetBuffer::BranchTargetBuffer()
    : entries{}
{
}

std::size_t BranchTargetBuffer::getIndex(uint32_t branchPC) const
{
    return (branchPC >> 2) % TABLE_SIZE;
}

bool BranchTargetBuffer::lookup(
    uint32_t branchPC,
    uint32_t& target
) const
{
    std::size_t index = getIndex(branchPC);
    const Entry& entry = entries[index];

    if (!entry.valid)
        return false;

    if (entry.tag != branchPC)
        return false;

    target = entry.target;
    return true;
}

void BranchTargetBuffer::update(
    uint32_t branchPC,
    uint32_t actualTarget
)
{
    std::size_t index = getIndex(branchPC);
    Entry& entry = entries[index];

    entry.valid = true;
    entry.tag = branchPC;
    entry.target = actualTarget;
}