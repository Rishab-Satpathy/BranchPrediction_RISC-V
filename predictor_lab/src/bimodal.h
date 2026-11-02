#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

class BimodalPredictor {

private:

    static constexpr std::size_t TABLE_SIZE = 16;

    std::array<uint8_t, TABLE_SIZE> counters;

    std::size_t getIndex(uint32_t branchPC) const;

public:

    BimodalPredictor();

    bool predict(uint32_t branchPC) const;

    void update(uint32_t branchPC, bool actualOutcome);
};