#include "../include/node.hpp"
#include "../include/util.hpp"
#include <stdexcept>

const float REGRET_MULTIPLIER = 150.0f;



void apply_equity_adjustments(std::vector<float>& distribution, const std::vector<Action>& valid_actions, float win_rate) {
    for (size_t i = 0; i < valid_actions.size(); i++) {
        char type = valid_actions[i].type;
        if (win_rate >= 0.9f) {
            if (type == 'f') distribution[i] *= 0.2f;
            else if (type == 'r') distribution[i] *= 3.0f;
            else if (type == 'a') distribution[i] *= 4.0f;
        } else if (win_rate >= 0.7f) {
            if (type == 'f') distribution[i] *= 0.5f;
            else if (type == 'r') distribution[i] *= 2.0f;
            else if (type == 'a') distribution[i] *= 2.0f;
        } else if (win_rate >= 0.4f) {
            if (type == 'c') distribution[i] *= 3.0f;
        } else {
            if (type == 'r') distribution[i] *= 0.3f;
            else if (type == 'a') distribution[i] *= 0.3f;
            else if (type == 'f') distribution[i] *= 2.0f;
        }
    }

    float normalizing_sum = 0;
    for (float val : distribution) {
        normalizing_sum += val;
    }
    if (normalizing_sum > 0) {
        for (float& val : distribution) {
            val /= normalizing_sum;
        }
    } else {
        throw std::runtime_error("apply_equity_adjustments: normalizing_sum is 0, this should never happen");
    }
}

Node::Node(const std::vector<Action>& valid_actions,
           const std::unordered_map<std::string, std::array<float, 2>>& equity_map,
           const std::string& infoset) {

    float uniform = 1.0f / valid_actions.size();
    std::vector<float> default_distribution(valid_actions.size(), uniform);

    std::string trimmed = trim_aggression_and_actions_off_infoset(infoset);
    auto it = equity_map.find(trimmed);
    if (it != equity_map.end() && it->second[1] > 0) { // check that the infoset exists and has been played at least once
        float win_rate = it->second[0] / it->second[1];
        apply_equity_adjustments(default_distribution, valid_actions, win_rate);
    }

    for (size_t i = 0; i < valid_actions.size(); i++) {
        const Action& a = valid_actions[i];
        actions.push_back(std::string(a));
        regret_sum.push_back(default_distribution[i] * REGRET_MULTIPLIER);
        strat_sum.push_back(0.0f);
        strat.push_back(default_distribution[i]);
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
