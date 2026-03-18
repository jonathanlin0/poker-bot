#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include <array>
#include <unordered_map>
#include <string>
#include <vector>
#include "node.hpp"
#include "util.hpp"

class Validation {
public:
    static void play_variants(
        const std::string& experiment_dir,
        int epoch,
        const std::vector<std::string>& variant_names,
        const std::array<std::unordered_map<std::string, Node>, 4>& nodes,
        const std::unordered_map<std::string, float>& infoset_to_hands_played,
        const EquityMap& equities
    );

    static float compute_exploitability(
        const std::array<std::unordered_map<std::string, Node>, 4>& nodes,
        const EquityMap& equities
    );
};

#endif // VALIDATION_HPP
