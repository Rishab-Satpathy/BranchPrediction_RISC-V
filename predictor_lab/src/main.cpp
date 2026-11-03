#include "bimodal.h"
#include "btb.h"
#include "gshare.h"

#include <bitset>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

struct BranchEvent {
    std::uint32_t pc = 0;
    bool taken = false;
    std::uint32_t target = 0;
};

struct SimulationMetrics {
    std::size_t totalBranches = 0;
    std::size_t directionCorrect = 0;
    std::size_t nextPcCorrect = 0;
    std::size_t btbHits = 0;
    std::size_t btbMisses = 0;

    std::size_t directionMispredictions() const {
        return totalBranches - directionCorrect;
    }
};

bool parseAddress(const std::string& text, std::uint32_t& output) {
    if (text.empty() || text.front() == '-') {
        return false;
    }

    try {
        std::size_t consumed = 0;
        const unsigned long long value = std::stoull(text, &consumed, 0);

        if (consumed != text.size() ||
            value > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }

        output = static_cast<std::uint32_t>(value);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

bool loadTrace(
    const std::string& filename,
    std::vector<BranchEvent>& trace
) {
    std::ifstream input(filename);
    if (!input) {
        std::cerr << "Error: could not open trace file '" << filename << "'.\n";
        return false;
    }

    trace.clear();
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        // Ignore blank lines and full-line comments (after optional whitespace).
        const std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') {
            continue;
        }

        std::istringstream parser(line);
        std::string pcText;
        std::string outcomeText;
        std::string targetText;
        std::string extraText;

        if (!(parser >> pcText >> outcomeText >> targetText) ||
            (parser >> extraText)) {
            std::cerr << "Error: malformed trace entry at " << filename
                      << ':' << lineNumber << ".\n";
            return false;
        }

        BranchEvent event;
        if (!parseAddress(pcText, event.pc) ||
            !parseAddress(targetText, event.target)) {
            std::cerr << "Error: invalid address at " << filename
                      << ':' << lineNumber << ".\n";
            return false;
        }

        if (outcomeText == "T") {
            event.taken = true;
        } else if (outcomeText == "N") {
            event.taken = false;
        } else {
            std::cerr << "Error: outcome must be T or N at " << filename
                      << ':' << lineNumber << ".\n";
            return false;
        }

        trace.push_back(event);
    }

    if (trace.empty()) {
        std::cerr << "Error: trace file contains no branch events: '"
                  << filename << "'.\n";
        return false;
    }

    return true;
}

std::string hexAddress(std::uint32_t address) {
    std::ostringstream output;
    output << "0x" << std::hex << std::uppercase
           << std::setw(8) << std::setfill('0') << address;
    return output.str();
}

std::string percentage(std::size_t numerator, std::size_t denominator) {
    const double value = denominator == 0
        ? 0.0
        : 100.0 * static_cast<double>(numerator) /
              static_cast<double>(denominator);

    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << value << '%';
    return output.str();
}

template <typename Predictor>
SimulationMetrics runSimulation(
    const std::string& predictorName,
    const std::vector<BranchEvent>& trace,
    bool verbose
) {
    Predictor predictor;
    BranchTargetBuffer btb;
    SimulationMetrics metrics;
    metrics.totalBranches = trace.size();

    if (verbose) {
        std::cout << "\n============================================================\n";
        std::cout << " " << predictorName << " PREDICTOR\n";
        std::cout << "============================================================\n";
    }

    for (const BranchEvent& event : trace) {
        // Capture the lookup state before prediction/training changes anything.
        const std::size_t pcIndex = predictor.debugPCIndex(event.pc);
        const std::size_t tableIndex = predictor.debugIndex(event.pc);
        const std::uint8_t counterBefore =
            predictor.debugCounterAt(tableIndex);

        std::uint8_t historyBefore = 0;
        if constexpr (std::is_same_v<Predictor, GsharePredictor>) {
            historyBefore = predictor.getGlobalHistory();
        }

        const bool predictedTaken = predictor.predict(event.pc);

        // The BTB is queried for every branch event, independently of direction.
        std::uint32_t btbTarget = 0;
        const bool btbHit = btb.lookup(event.pc, btbTarget);

        const std::uint32_t sequentialPC = event.pc + 4;
        std::uint32_t predictedNextPC = sequentialPC;
        if (predictedTaken && btbHit) {
            predictedNextPC = btbTarget;
        }

        const std::uint32_t actualNextPC =
            event.taken ? event.target : sequentialPC;

        const bool directionCorrect = predictedTaken == event.taken;
        const bool nextPcCorrect = predictedNextPC == actualNextPC;

        if (directionCorrect) {
            ++metrics.directionCorrect;
        }
        if (nextPcCorrect) {
            ++metrics.nextPcCorrect;
        }
        if (btbHit) {
            ++metrics.btbHits;
        } else {
            ++metrics.btbMisses;
        }

        // Train after evaluating this prediction. Taken branches install a BTB target.
        predictor.update(event.pc, event.taken);
        if (event.taken) {
            btb.update(event.pc, event.target);
        }

        const std::uint8_t counterAfter =
            predictor.debugCounterAt(tableIndex);
        std::uint8_t historyAfter = 0;
        if constexpr (std::is_same_v<Predictor, GsharePredictor>) {
            historyAfter = predictor.getGlobalHistory();
        }

        if (verbose) {
            std::cout << "PC=" << hexAddress(event.pc);

            if constexpr (std::is_same_v<Predictor, GsharePredictor>) {
                std::cout << " GHR=" << std::bitset<4>(historyBefore);
            } else {
                std::cout << " GHR=----";
            }

            std::cout << " PCIdx=" << std::setw(2) << pcIndex
                      << " Idx=" << std::setw(2) << tableIndex
                      << " Ctr=" << std::bitset<2>(counterBefore)
                      << "->" << std::bitset<2>(counterAfter)
                      << " Pred=" << (predictedTaken ? 'T' : 'N')
                      << " Act=" << (event.taken ? 'T' : 'N')
                      << " BTB=" << (btbHit ? "HIT " : "MISS")
                      << " PredNext=" << hexAddress(predictedNextPC)
                      << " ActNext=" << hexAddress(actualNextPC)
                      << " NextPC=" << (nextPcCorrect ? "OK" : "ERR");

            if constexpr (std::is_same_v<Predictor, GsharePredictor>) {
                std::cout << " GHRAfter=" << std::bitset<4>(historyAfter);
            }
            std::cout << '\n';
        }
    }

    if (verbose) {
        std::cout << "\n" << predictorName << " SUMMARY\n";
        std::cout << "Total branches:       " << metrics.totalBranches << '\n';
        std::cout << "Direction correct:    " << metrics.directionCorrect
                  << '/' << metrics.totalBranches << " ("
                  << percentage(metrics.directionCorrect, metrics.totalBranches)
                  << ")\n";
        std::cout << "Mispredictions:       "
                  << metrics.directionMispredictions() << " ("
                  << percentage(metrics.directionMispredictions(),
                                 metrics.totalBranches)
                  << ")\n";
        std::cout << "Next-PC correct:      " << metrics.nextPcCorrect
                  << '/' << metrics.totalBranches << " ("
                  << percentage(metrics.nextPcCorrect, metrics.totalBranches)
                  << ")\n";
        std::cout << "BTB lookup hits:      " << metrics.btbHits << '\n';
        std::cout << "BTB lookup misses:    " << metrics.btbMisses << '\n';
        std::cout << "BTB lookup hit rate:  "
                  << percentage(metrics.btbHits, metrics.totalBranches) << '\n';
    }

    return metrics;
}

void printComparison(
    const std::string& traceName,
    const SimulationMetrics& bimodal,
    const SimulationMetrics& gshare
) {
    const char* winner = "Tie";
    if (gshare.directionCorrect > bimodal.directionCorrect) {
        winner = "Gshare";
    } else if (bimodal.directionCorrect > gshare.directionCorrect) {
        winner = "Bimodal";
    }

    std::cout << "\nComparison for " << traceName << '\n';
    std::cout << std::left << std::setw(13) << "Predictor"
              << std::right << std::setw(13) << "Direction"
              << std::setw(13) << "Mispredict"
              << std::setw(13) << "Next-PC"
              << std::setw(13) << "BTB hits" << '\n';

    const auto printRow = [](const char* name, const SimulationMetrics& m) {
        std::cout << std::left << std::setw(13) << name
                  << std::right << std::setw(9)
                  << m.directionCorrect << '/' << m.totalBranches
                  << std::setw(9)
                  << percentage(m.directionMispredictions(), m.totalBranches)
                  << std::setw(9)
                  << m.nextPcCorrect << '/' << m.totalBranches
                  << std::setw(9)
                  << m.btbHits << '/' << m.totalBranches << '\n';
    };

    printRow("Bimodal", bimodal);
    printRow("Gshare", gshare);
    std::cout << "Direction-accuracy winner: " << winner << "\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> traceFiles;
    bool runAll = false;

    if (argc == 2 && std::string(argv[1]) == "--all") {
        runAll = true;
        traceFiles = {
            "traces/loop.trace",
            "traces/two_branches.trace",
            "traces/correlated.trace",
            "traces/alternating.trace",
            "traces/aliasing.trace",
            "traces/btb_conflict.trace"
        };
    } else if (argc == 2) {
        traceFiles.emplace_back(argv[1]);
    } else {
        std::cerr << "Usage:\n"
                  << "  predictor_lab.exe <trace-file>\n"
                  << "  predictor_lab.exe --all\n\n"
                  << "Run these commands from the predictor_lab directory.\n";
        return 1;
    }

    for (const std::string& filename : traceFiles) {
        std::vector<BranchEvent> trace;
        if (!loadTrace(filename, trace)) {
            return 1;
        }

        std::cout << "\nLoaded " << trace.size()
                  << " branch events from " << filename << ".\n";

        // Each predictor gets a fresh instance and a fresh BTB.
        const bool verbose = !runAll;
        const SimulationMetrics bimodal =
            runSimulation<BimodalPredictor>("BIMODAL", trace, verbose);
        const SimulationMetrics gshare =
            runSimulation<GsharePredictor>("GSHARE", trace, verbose);

        printComparison(filename, bimodal, gshare);
    }

    return 0;
}
