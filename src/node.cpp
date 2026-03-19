#include "../include/node.hpp"
#include "../include/initial_strategy_getter.hpp"
#include "../include/util.hpp"
#include <stdexcept>

using std::vector;

const float REGRET_MULTIPLIER = 150.0f;

Node::Node(uint8_t street,
           const std::string& infoset,
           const std::vector<Action>& valid_actions) {

    vector<float> initial_strategy = InitialStrategyGetter::get_initial_strategy(street, infoset, valid_actions);

    for (size_t i = 0; i < valid_actions.size(); i++) {
        const Action& a = valid_actions[i];
        actions.push_back(std::string(a));
        regret_sum.push_back(initial_strategy[i] * REGRET_MULTIPLIER);
        strat_sum.push_back(0.0f);
        strat.push_back(initial_strategy[i]);
        avg_strat.push_back(0.0f);
    }
}

size_t Node::action_index(const std::string& action) const {
    for (size_t i = 0; i < actions.size(); i++) {
        if (actions[i] == action) return i;
    }
    throw std::runtime_error("Action not found in node: " + action);
}

std::unordered_map<std::string, float> Node::get_avg_strat_map() const {
    std::unordered_map<std::string, float> map;
    for (size_t i = 0; i < actions.size(); i++) {
        map[actions[i]] = avg_strat[i];
    }
    return map;
}
