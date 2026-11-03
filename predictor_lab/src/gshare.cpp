#include "gshare.h"

#include <stdexcept>

GsharePredictor::GsharePredictor() {
    // Match Bimodal's starting counter state for a fair comparison.
    counters.fill(1);
    globalHistory = 0;
}

std::size_t GsharePredictor::getIndex(std::uint32_t branchPC) const {
    const std::uint32_t pcIndex =
        static_cast<std::uint32_t>((branchPC >> 2) & HISTORY_MASK);

    // Gshare combines the PC index with recent global branch outcomes.
    return static_cast<std::size_t>(pcIndex ^ globalHistory);
}

bool GsharePredictor::predict(std::uint32_t branchPC) const {
    const std::uint8_t counter = counters[getIndex(branchPC)];
    return counter >= 2;
}

void GsharePredictor::update(std::uint32_t branchPC, bool actualOutcome) {
    // IMPORTANT: calculate the table index before modifying the GHR.
    const std::size_t index = getIndex(branchPC);
    std::uint8_t& counter = counters[index];

    if (actualOutcome) {
        if (counter < 3) {
            ++counter;
        }
    } else {
        if (counter > 0) {
            --counter;
        }
    }

    // Shift left, insert the newest outcome into bit 0, keep four history bits.
    globalHistory = static_cast<std::uint8_t>(
        ((globalHistory << 1) | (actualOutcome ? 1u : 0u)) & HISTORY_MASK
    );
}

std::uint8_t GsharePredictor::getGlobalHistory() const {
    return globalHistory;
}

std::size_t GsharePredictor::debugPCIndex(std::uint32_t branchPC) const {
    return static_cast<std::size_t>((branchPC >> 2) & HISTORY_MASK);
}

std::size_t GsharePredictor::debugIndex(std::uint32_t branchPC) const {
    return getIndex(branchPC);
}

std::uint8_t GsharePredictor::debugCounterAt(std::size_t index) const {
    if (index >= TABLE_SIZE) {
        throw std::out_of_range("Gshare counter index is out of range");
    }
    return counters[index];
}
