#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class GsharePredictor {
private:
    static constexpr std::size_t TABLE_SIZE = 16;
    static constexpr std::uint8_t HISTORY_MASK =
        static_cast<std::uint8_t>(TABLE_SIZE - 1);

    std::array<std::uint8_t, TABLE_SIZE> counters{};
    std::uint8_t globalHistory = 0;

    std::size_t getIndex(std::uint32_t branchPC) const;

public:
    GsharePredictor();

    // Returns true for Taken and false for Not Taken.
    bool predict(std::uint32_t branchPC) const;

    // Trains the counter using the OLD GHR, then shifts in the actual outcome.
    void update(std::uint32_t branchPC, bool actualOutcome);

    // Read-only debug helpers used by the simulator.
    std::uint8_t getGlobalHistory() const;
    std::size_t debugPCIndex(std::uint32_t branchPC) const;
    std::size_t debugIndex(std::uint32_t branchPC) const;
    std::uint8_t debugCounterAt(std::size_t index) const;
};
