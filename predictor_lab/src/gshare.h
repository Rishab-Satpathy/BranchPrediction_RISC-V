#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

class GsharePredictor {

private:

    static constexpr std::size_t TABLE_SIZE = 16;
    static constexpr uint8_t HISTORY_MASK = TABLE_SIZE - 1;

    std::array<uint8_t, TABLE_SIZE> counters;

    uint8_t globalHistory;

    std::size_t getIndex(uint32_t branchPC) const;

public:

    GsharePredictor();

    bool predict(uint32_t branchPC) const;

    void update(uint32_t branchPC, bool actualOutcome);
};