#include "gshare.h"

GsharePredictor::GsharePredictor()
{
    counters.fill(1);

    globalHistory = 0;
}

std::size_t GsharePredictor::getIndex(uint32_t branchPC) const
{
    uint32_t pcIndex = (branchPC >> 2) & HISTORY_MASK;

    return static_cast<std::size_t>(
        pcIndex ^ globalHistory
    );
}

bool GsharePredictor::predict(uint32_t branchPC) const
{
    std::size_t index = getIndex(branchPC);

    uint8_t counter = counters[index];

    return counter >= 2;
}

void GsharePredictor::update(
    uint32_t branchPC,
    bool actualOutcome
)
{
    // 1. Find the counter using the current history.
    std::size_t index = getIndex(branchPC);

    uint8_t& counter = counters[index];

    // 2. Update the selected saturating counter.
    if (actualOutcome)
    {
        if (counter < 3)
            counter++;
    }
    else
    {
        if (counter > 0)
            counter--;
    }

    // 3. Shift in the actual branch outcome.
    globalHistory = static_cast<uint8_t>(
        ((globalHistory << 1) |
        (actualOutcome ? 1u : 0u)) & HISTORY_MASK
    );
}