#include "bimodal.h"

#include <stdexcept>

BimodalPredictor::BimodalPredictor() {
    // 01 = weakly Not Taken. Both predictors use the same initial state.
    counters.fill(1);
}

std::size_t BimodalPredictor::getIndex(std::uint32_t branchPC) const {
    // Instructions are 4-byte aligned, so discard the two lowest address bits.
    // A 16-entry table needs four index bits.
    return static_cast<std::size_t>((branchPC >> 2) & (TABLE_SIZE - 1));
}

bool BimodalPredictor::predict(std::uint32_t branchPC) const {
    const std::uint8_t counter = counters[getIndex(branchPC)];
    return counter >= 2;
}

void BimodalPredictor::update(std::uint32_t branchPC, bool actualOutcome) {
    std::uint8_t& counter = counters[getIndex(branchPC)];

    if (actualOutcome) {
        if (counter < 3) {
            ++counter;
        }
    } else {
        if (counter > 0) {
            --counter;
        }
    }
}

std::size_t BimodalPredictor::debugPCIndex(std::uint32_t branchPC) const {
    return static_cast<std::size_t>((branchPC >> 2) & (TABLE_SIZE - 1));
}

std::size_t BimodalPredictor::debugIndex(std::uint32_t branchPC) const {
    return getIndex(branchPC);
}

std::uint8_t BimodalPredictor::debugCounterAt(std::size_t index) const {
    if (index >= TABLE_SIZE) {
        throw std::out_of_range("Bimodal counter index is out of range");
    }
    return counters[index];
}
