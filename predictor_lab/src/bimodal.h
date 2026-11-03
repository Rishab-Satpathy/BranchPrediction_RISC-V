#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class BimodalPredictor {
private:
    static constexpr std::size_t TABLE_SIZE = 16;
    std::array<std::uint8_t, TABLE_SIZE> counters{};

    std::size_t getIndex(std::uint32_t branchPC) const;

public:
    BimodalPredictor();

    // Returns true for Taken and false for Not Taken.
    bool predict(std::uint32_t branchPC) const;

    // Trains the counter selected by this branch PC.
    void update(std::uint32_t branchPC, bool actualOutcome);

    // Read-only debug helpers used by the simulator.
    std::size_t debugPCIndex(std::uint32_t branchPC) const;
    std::size_t debugIndex(std::uint32_t branchPC) const;
    std::uint8_t debugCounterAt(std::size_t index) const;
};
