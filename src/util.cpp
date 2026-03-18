#include "../include/util.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <string>

const char SUITS[] = {'H', 'S', 'D', 'C'};
const char RANKS[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};

static thread_local std::mt19937 rng(std::random_device{}());
static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

std::vector<Card> get_new_deck(bool shuffle) {
    std::vector<Card> deck;
    for (char suit : SUITS) {
        for (char rank : RANKS) {
            deck.push_back(Card{suit, rank});
        }
    }
    if (shuffle) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(deck.begin(), deck.end(), gen);
    }
    return deck;
}


static size_t count_num_of_delimiters(const std::string& s, const std::string& delimiter) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = s.find(delimiter, pos)) != std::string::npos) {
        count++;
        pos += delimiter.size();
    }
    return count;
}

std::string trim_aggression_and_actions_off_infoset(const std::string& infoset) {
    const std::string bucket_delimiter = " ";

    // Preflop infosets have 4 delimiters, postflop have 5 (board_texture adds one).
    // If this count changes, the infoset format changed and this function needs updating.
    size_t count = count_num_of_delimiters(infoset, bucket_delimiter);
    if (count != 4 && count != 5) {
        throw std::runtime_error(
            "trim_aggression_and_actions_off_infoset: expected 4 (preflop) or 5 (postflop) delimiters, got "
            + std::to_string(count) + " in: " + infoset);
    }

    size_t last = infoset.rfind(bucket_delimiter);
    size_t second_last = infoset.rfind(bucket_delimiter, last - 1);
    return infoset.substr(0, second_last);
}

EquityMap load_precomputed_equities() {
    EquityMap data{};
    const std::string filepath = "data/precomputed_equity.txt";
    const std::string bucket_delimiter = " ";
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Warning: could not load precomputed equities from " << filepath << std::endl;
        return data;
    }

    std::string infoset, wins_str, total_str;
    while (std::getline(file, infoset)) {
        if (!std::getline(file, wins_str) || !std::getline(file, total_str)) break;

        size_t first_delim = infoset.find(bucket_delimiter);
        size_t second_delim = infoset.find(bucket_delimiter, first_delim + 1);
        int street = std::stoi(infoset.substr(first_delim + 1, second_delim - first_delim - 1));

        data[street][infoset][0] += std::stof(wins_str);
        data[street][infoset][1] += std::stof(total_str);
    }

    size_t total = 0;
    for (int s = 0; s < 4; s++) total += data[s].size();
    std::cout << "Loaded precomputed equities: " << total << " infosets from " << filepath << std::endl;
    return data;
}

size_t sample_from_distribution_list(const std::vector<float>& distribution) {
    assert(!distribution.empty() && "Distribution is empty");
    float sum = 0.0f;
    for (float prob : distribution) {
        sum += prob;
    }
    assert(std::fabs(sum - 1.0f) < 1e-5f && "Distribution does not sum to 1.0");

    float r = dist(rng);
    float cumulative = 0.0f;
    for (size_t i = 0; i < distribution.size(); i++) {
        cumulative += distribution[i];
        if (r <= cumulative) {
            return i;
        }
    }
    return distribution.size() - 1;
}
