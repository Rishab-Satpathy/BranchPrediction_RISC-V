#include "bimodal.h"

BimodalPredictor::BimodalPredictor()
{
    counters.fill(1);
}

std::size_t BimodalPredictor::getIndex(uint32_t branchPC) const
{
    return (branchPC >> 2) % TABLE_SIZE;
}

bool BimodalPredictor::predict(uint32_t branchPC) const
{
    std::size_t index = getIndex(branchPC);

    uint8_t counter = counters[index];

    return counter >= 2;
}

void BimodalPredictor::update(
    uint32_t branchPC,
    bool actualOutcome
)
{
    std::size_t index = getIndex(branchPC);

    uint8_t& counter = counters[index];

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
}