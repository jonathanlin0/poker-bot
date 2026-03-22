#include "../include/cfr_util.hpp"

void calculate_avg_strat(std::array<std::unordered_map<std::string, Node>, 4>& nodes) {
    for (int street = 0; street < 4; street++) {
        for (auto& [infoset, node] : nodes[street]) {
            float normalizing_sum = 0.0f;
            for (float val : node.strat_sum) {
                normalizing_sum += val;
            }

            if (normalizing_sum > 0) {
                for (size_t i = 0; i < node.actions.size(); i++) {
                    node.avg_strat[i] = node.strat_sum[i] / normalizing_sum;
                }
            } else {
                float uniform_prob = 1.0f / node.actions.size();
                for (size_t i = 0; i < node.actions.size(); i++) {
                    node.avg_strat[i] = uniform_prob;
                }
            }
        }
    }
}
