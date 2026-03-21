#ifndef NODE_HPP
#define NODE_HPP

#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include "action.hpp"

struct Node {
    std::vector<std::string> actions;
    std::vector<float> regret_sum;
    std::vector<float> strat_sum;
    std::vector<float> strat;
    std::vector<float> avg_strat;

    Node() = default;
    Node(
        uint8_t street,
        const std::string& infoset,
        const std::vector<Action>& valid_actions
    );

    size_t action_index(const std::string& action) const;
    std::unordered_map<std::string, float> get_avg_strat_map() const;
};

#endif // NODE_HPP
