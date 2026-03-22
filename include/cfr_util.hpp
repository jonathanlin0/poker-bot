#ifndef CFR_UTIL_HPP
#define CFR_UTIL_HPP

#include "node.hpp"
#include <array>
#include <string>
#include <unordered_map>

void calculate_avg_strat(std::array<std::unordered_map<std::string, Node>, 4>& nodes);

#endif // CFR_UTIL_HPP
