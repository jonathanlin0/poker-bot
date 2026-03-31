#ifndef CONFIG_H
#define CONFIG_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// bot version (stored in DB with each hand)
const std::string BOT_VERSION = "1.0.0";

// number of epochs ignored for updating strat sum
const int AVERAGING_DELAY = 1000;

// how often to save the nodes to a file
const int SAVE_INTERVAL = 100000;


// game constants
const uint16_t STARTING_STACK = 200; // in SB (= 100 BB)
const int MAX_RAISES_PER_STREET = 2;

// preflop: multiples of big blind
const std::array<uint8_t, 5> PREFLOP_RAISE_SIZES = {2, 4, 8, 16, 32};
// postflop: fractions of the pot
const std::array<float, 5> POSTFLOP_RAISE_SIZES = {0.3f, 0.5f, 1.0f, 1.5f, 2.0f};


// validation constants
// the variants that the bot plays against for val
const std::vector<std::string> VARIANT_NAMES = {
    "over-call",
    "agro",
    "tight",
    "unif",
    "itself",
    "tight-agro", 
    "initial-weights"
};

// how many epochs between each validation check
const int VALIDATION_INTERVAL = 5000;
// how many hands to play for each variant in validation
const int HANDS_PER_VARIANT = 100000;
// how many hands to simulate for tight-agro variant to check win rate
const int TIGHT_AGRO_NUM_SIMS = 5;

// how many samples to take to compute exploitability
// multiply by 2 because it's done for both players
const int EXPLOITABILITY_NUM_SAMPLES = 2000;
// how many threads to use to compute exploitability
const int NUM_EXPLOITABILITY_THREADS = 10;


// precomputed equities
const std::string PRECOMPUTED_EQUITIES_FILE = "data/precomputed_equity.txt";

// how many hands each thread simulates (check/call to showdown) to estimate win rates per infoset
const int PRECOMPUTE_EQUITY_HANDS_PER_THREAD = 5000000;
// how often each thread prints progress (in hands)
const int PRECOMPUTE_EQUITY_PRINT_INTERVAL = 25000;
// number of threads to run in parallel for equity precomputation
const int PRECOMPUTE_EQUITY_NUM_THREADS = 20;

#endif
