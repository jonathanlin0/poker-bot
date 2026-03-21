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

#endif // SERIALIZATION_HPP
