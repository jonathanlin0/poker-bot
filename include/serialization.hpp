#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <array>
#include <string>
#include <unordered_map>
#include "node.hpp"

/*
    Saves the nodes to a file
*/
void save_nodes(
    const std::string& filepath,
    const std::array<std::unordered_map<std::string, Node>,
    4>& nodes
);

/*
    Loads the nodes from a file
*/
void load_nodes(
    const std::string& filepath, 
    std::array<std::unordered_map<std::string, Node>,
    4>& nodes
);

/*
    Loads only avg_strat data from avg_strat.bin (actions + avg_strat per node).
    Lighter than load_nodes - skips regret_sum/strat_sum.
*/
void load_avg_strat(
    const std::string& filepath,
    std::array<std::unordered_map<std::string, Node>,
    4>& nodes
);

#endif // SERIALIZATION_HPP
