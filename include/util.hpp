#ifndef UTIL_HPP
#define UTIL_HPP

#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include "card.hpp"

// [0] = wins (ties count as 0.5), [1] = total games
using EquityMap = std::array<std::unordered_map<std::string, std::array<float, 2>>, 4>;

std::vector<Card> get_new_deck(bool shuffle = true);

// Samples an index from a probability vector, returns the chosen index
size_t sample_from_distribution_list(const std::vector<float>& distribution);

/*
    Removes the last 2 bucket-delimited sections (betting aggression and possible actions) from an infoset string
*/
std::string trim_aggression_and_actions_off_infoset(const std::string& infoset);

#endif // UTIL_HPP
