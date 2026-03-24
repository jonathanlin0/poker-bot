#include "../include/serialization.hpp"
#include <fstream>
#include <stdexcept>

/*
    weights.bin — Binary format (per street):
        [num_infosets: u32]
        For each infoset:
            [key_len: u32][key_bytes...]
            [num_actions: u32]
            For each action:
                [action_len: u32][action_bytes...]
            [regret_sum: num_actions floats]
            [strat_sum: num_actions floats]

    avg_strat.bin — Binary format (per street):
        [num_infosets: u32]
        For each infoset:
            [key_len: u32][key_bytes...]
            [num_actions: u32]
            For each action:
                [action_len: u32][action_bytes...]
            [avg_strat: num_actions floats]

    avg_strat can be derived from strat_sum, but it's saved for convenience
    for clearer weight loading for inference (playing against the bot's weights).
*/

// avg_strat can be derived from strat_sum, but it's saved for convenience
// for clearer weight loading for inference (playing against the bot's weights).
// TODO: write tests for this function
static void save_avg_strat(const std::string& experiment_dir, const std::array<std::unordered_map<std::string, Node>, 4>& nodes) {
    std::string filepath = experiment_dir + "/avg_strat.bin";
    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }

    for (int street = 0; street < 4; street++) {
        uint32_t num_infosets = nodes[street].size();
        out.write(reinterpret_cast<const char*>(&num_infosets), sizeof(num_infosets));

        for (const auto& [key, node] : nodes[street]) {
            uint32_t key_len = key.size();
            out.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            out.write(key.data(), key_len);

            uint32_t num_actions = node.actions.size();
            out.write(reinterpret_cast<const char*>(&num_actions), sizeof(num_actions));

            for (const auto& action : node.actions) {
                uint32_t action_len = action.size();
                out.write(reinterpret_cast<const char*>(&action_len), sizeof(action_len));
                out.write(action.data(), action_len);
            }

            out.write(reinterpret_cast<const char*>(node.avg_strat.data()), num_actions * sizeof(float));
        }
    }
}

void save_nodes(const std::string& experiment_dir, const std::array<std::unordered_map<std::string, Node>, 4>& nodes) {
    // Ensure that each street has at least one infoset
    for (int street = 0; street < 4; street++) {
        if (nodes[street].empty()) {
            throw std::runtime_error("Cannot save nodes: street " + std::to_string(street) + " has 0 infosets");
        }
    }

    std::string filepath = experiment_dir + "/weights.bin";

    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }

    for (int street = 0; street < 4; street++) {
        
        // Write how many infosets this street has so the loader knows how many to read
        uint32_t num_infosets = nodes[street].size();
        out.write(reinterpret_cast<const char*>(&num_infosets), sizeof(num_infosets));

        for (const auto& [key, node] : nodes[street]) {
            // Write infoset key: length prefix followed by the string bytes
            uint32_t key_len = key.size();
            out.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            out.write(key.data(), key_len);

            // Write action strings with the same length-prefix pattern
            uint32_t num_actions = node.actions.size();
            out.write(reinterpret_cast<const char*>(&num_actions), sizeof(num_actions));

            for (const auto& action : node.actions) {
                uint32_t action_len = action.size();
                out.write(reinterpret_cast<const char*>(&action_len), sizeof(action_len));
                out.write(action.data(), action_len);
            }

            // Write the float arrays as raw bytes (contiguous in memory)
            out.write(reinterpret_cast<const char*>(node.regret_sum.data()), num_actions * sizeof(float));
            out.write(reinterpret_cast<const char*>(node.strat_sum.data()), num_actions * sizeof(float));
        }
    }

    save_avg_strat(experiment_dir, nodes);
}

void load_avg_strat(const std::string& filepath, std::array<std::unordered_map<std::string, Node>, 4>& nodes) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filepath);
    }

    for (int street = 0; street < 4; street++) {
        uint32_t num_infosets;
        in.read(reinterpret_cast<char*>(&num_infosets), sizeof(num_infosets));

        for (uint32_t i = 0; i < num_infosets; i++) {
            uint32_t key_len;
            in.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            std::string key(key_len, '\0');
            in.read(key.data(), key_len);

            Node node;

            uint32_t num_actions;
            in.read(reinterpret_cast<char*>(&num_actions), sizeof(num_actions));

            node.actions.resize(num_actions);
            for (uint32_t j = 0; j < num_actions; j++) {
                uint32_t action_len;
                in.read(reinterpret_cast<char*>(&action_len), sizeof(action_len));
                node.actions[j].resize(action_len);
                in.read(node.actions[j].data(), action_len);
            }

            node.avg_strat.resize(num_actions);
            in.read(reinterpret_cast<char*>(node.avg_strat.data()), num_actions * sizeof(float));

            nodes[street][key] = std::move(node);
        }
    }
}

void load_nodes(const std::string& filepath, std::array<std::unordered_map<std::string, Node>, 4>& nodes) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filepath);
    }

    for (int street = 0; street < 4; street++) {
        uint32_t num_infosets;
        in.read(reinterpret_cast<char*>(&num_infosets), sizeof(num_infosets));

        for (uint32_t i = 0; i < num_infosets; i++) {
            // Read infoset key: read the length, then read that many bytes into a string
            uint32_t key_len;
            in.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            std::string key(key_len, '\0');
            in.read(key.data(), key_len);

            Node node;

            // Read action strings with the same length-prefix pattern
            uint32_t num_actions;
            in.read(reinterpret_cast<char*>(&num_actions), sizeof(num_actions));

            node.actions.resize(num_actions);
            for (uint32_t j = 0; j < num_actions; j++) {
                uint32_t action_len;
                in.read(reinterpret_cast<char*>(&action_len), sizeof(action_len));
                node.actions[j].resize(action_len);
                in.read(node.actions[j].data(), action_len);
            }

            // Allocate float vectors and read raw bytes directly into them
            node.regret_sum.resize(num_actions);
            node.strat_sum.resize(num_actions);
            node.strat.resize(num_actions, 0.0f);
            node.avg_strat.resize(num_actions, 0.0f);

            in.read(reinterpret_cast<char*>(node.regret_sum.data()), num_actions * sizeof(float));
            in.read(reinterpret_cast<char*>(node.strat_sum.data()), num_actions * sizeof(float));

            nodes[street][key] = std::move(node);
        }
    }
}
