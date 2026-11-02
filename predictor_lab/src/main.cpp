#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "bimodal.h"
#include "btb.h"

using namespace std;

struct BranchEvent {
    std::uint32_t pc{};
    bool taken{};
    std::uint32_t target{};
};


bool parseAddress(const string& text, uint32_t& out) {

    // 1. Reject empty strings and negative addresses.
    if (text.empty() || text[0] == '-') {
        return false;
    }

    // 2. Tracks how many characters stoull successfully parses.
    size_t consumed = 0;

    try {

        // 3. Convert the string into a large unsigned integer.
        unsigned long long value = stoull(text, &consumed, 0);

        // 4. Ensure the ENTIRE string was converted.
        if (consumed != text.size()) {
            return false;
        }

        // 5. Ensure the value fits in 32 bits.
        if (value > numeric_limits<uint32_t>::max()) {
            return false;
        }

        // 6. Conversion is valid. Store the address.
        out = static_cast<uint32_t>(value);

        return true;
    }

    // 7. No valid number could be extracted.
    catch (const invalid_argument& e) {
        return false;
    }

    // 8. Number exceeded the range of unsigned long long.
    catch (const out_of_range& e) {
        return false;
    }
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: predictor_lab <trace-file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Could not open trace file\n";
        return 1;
    }

    std::vector<BranchEvent> trace;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream row(line);
        std::string pcText, outcomeText, targetText, extra;

        if (!(row >> pcText >> outcomeText >> targetText) ||
            (row >> extra)) {
            std::cerr << "Malformed line " << lineNumber << '\n';
            return 1;
        }

        if (outcomeText != "T" && outcomeText != "N") {
            std::cerr << "Invalid outcome on line "
                      << lineNumber << '\n';
            return 1;
        }

        std::uint32_t pc{}, target{};

        if (!parseAddress(pcText, pc) ||
            !parseAddress(targetText, target)) {
            std::cerr << "Invalid address on line "
                      << lineNumber << '\n';
            return 1;
        }

        trace.push_back({pc, outcomeText == "T", target});
    }

    std::cout << "Loaded " << trace.size()
              << " branch events\n";

    for (const auto& event : trace) {
        std::cout << std::hex << "PC=0x" << event.pc
                  << " Outcome=" << (event.taken ? 'T' : 'N')
                  << " Target=0x" << event.target << '\n';
    }

    BimodalPredictor predictor;

int correct = 0;
int total = 0;

BimodalPredictor bht;
BranchTargetBuffer btb;

int directionCorrect = 0;
int nextPCCorrect = 0;

int btbHits = 0;
int btbMisses = 0;

for (const BranchEvent& event : trace)
{
    // 1. Predict branch direction using the BHT.
    bool predictedTaken = bht.predict(event.pc);

    // 2. Look up the target using the BTB.
    uint32_t predictedTarget = 0;

    bool btbHit = btb.lookup(event.pc, predictedTarget);

    if (btbHit)
        btbHits++;
    else
        btbMisses++;

    // 3. Calculate the predicted next PC.
    uint32_t fallThroughPC = event.pc + 4;

    uint32_t predictedNextPC;

    if (predictedTaken && btbHit)
        predictedNextPC = predictedTarget;
    else
        predictedNextPC = fallThroughPC;

    // 4. Calculate what the actual next PC should have been.
    uint32_t actualNextPC;

    if (event.taken)
        actualNextPC = event.target;
    else
        actualNextPC = fallThroughPC;

    // 5. Evaluate direction prediction.
    bool directionIsCorrect =
        (predictedTaken == event.taken);

    if (directionIsCorrect)
        directionCorrect++;

    // 6. Evaluate the complete next-PC prediction.
    bool nextPCIsCorrect =
        (predictedNextPC == actualNextPC);

    if (nextPCIsCorrect)
        nextPCCorrect++;

    total++;

    // 7. Print the results.
    std::cout << std::hex
              << "PC=0x" << event.pc
              << " ActualNextPC=0x" << actualNextPC
              << " PredictedNextPC=0x" << predictedNextPC
              << " Direction=" << (predictedTaken ? "T" : "N")
              << " BTB=" << (btbHit ? "HIT" : "MISS")
              << " NextPC=" << (nextPCIsCorrect ? "CORRECT" : "WRONG")
              << '\n';

    // 8. Train the predictors AFTER making the prediction.
    bht.update(event.pc, event.taken);

    if (event.taken)
        btb.update(event.pc, event.target);
}

// 9. Print statistics.
std::cout << std::dec;

std::cout << "\nDirection accuracy: "
          << directionCorrect << "/" << total
          << " = "
          << (100.0 * directionCorrect / total)
          << "%\n";

std::cout << "Next-PC accuracy: "
          << nextPCCorrect << "/" << total
          << " = "
          << (100.0 * nextPCCorrect / total)
          << "%\n";

std::cout << "BTB hits: " << btbHits << '\n';
std::cout << "BTB misses: " << btbMisses << '\n';

double accuracy = 100.0 * correct / total;

std::cout << std::dec
          << "\nCorrect predictions: " << correct << "/" << total
          << "\nAccuracy: " << accuracy << "%\n";

    return 0;
}