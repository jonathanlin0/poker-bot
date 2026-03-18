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
    Node(const std::vector<Action>& valid_actions,
         const std::unordered_map<std::string, std::array<float, 2>>& equity_map,
         const std::string& infoset);

    size_t action_index(const std::string& action) const;
    std::unordered_map<std::string, float> get_avg_strat_map() const;
};

void apply_equity_adjustments(std::vector<float>& distribution, const std::vector<Action>& valid_actions, float win_rate);

#endif // NODE_HPP
