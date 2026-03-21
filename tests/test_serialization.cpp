#include <gtest/gtest.h>
#include <filesystem>
#include "serialization.hpp"
#include "node.hpp"

namespace fs = std::filesystem;

static Node make_test_node(const std::vector<std::string>& actions,
                           const std::vector<float>& regret_sum,
                           const std::vector<float>& strat_sum) {
    Node node;
    node.actions = actions;
    node.regret_sum = regret_sum;
    node.strat_sum = strat_sum;
    node.strat.resize(actions.size(), 0.0f);
    node.avg_strat.resize(actions.size(), 0.0f);
    return node;
}

class SerializationTest : public ::testing::Test {
protected:
    std::string test_filepath = "tests/test_weights_tmp.bin";

    void TearDown() override {
        fs::remove(test_filepath);
    }
};

// Save nodes across all 4 streets, load them back, and verify every field matches exactly
TEST_F(SerializationTest, RoundTripPreservesData) {
    std::array<std::unordered_map<std::string, Node>, 4> original;

    original[0]["preflop_infoset_1"] = make_test_node(
        {"c", "r10;0.50", "f"},
        {1.5f, 3.0f, 0.5f},
        {10.0f, 20.0f, 5.0f}
    );
    original[0]["preflop_infoset_2"] = make_test_node(
        {"c", "f"},
        {2.0f, 1.0f},
        {15.0f, 8.0f}
    );
    original[1]["flop_infoset"] = make_test_node(
        {"c", "r20;1.00", "a"},
        {0.0f, 5.0f, 2.5f},
        {0.0f, 30.0f, 12.0f}
    );
    original[2]["turn_infoset"] = make_test_node(
        {"c", "f"},
        {4.0f, 1.0f},
        {25.0f, 10.0f}
    );
    original[3]["river_infoset"] = make_test_node(
        {"c", "r50;2.00"},
        {3.0f, 7.0f},
        {18.0f, 42.0f}
    );

    save_nodes(test_filepath, original);

    std::array<std::unordered_map<std::string, Node>, 4> loaded;
    load_nodes(test_filepath, loaded);

    for (int street = 0; street < 4; street++) {
        ASSERT_EQ(loaded[street].size(), original[street].size()) << "Street " << street;

        for (const auto& [key, orig_node] : original[street]) {
            ASSERT_TRUE(loaded[street].count(key)) << "Missing key: " << key;
            const Node& load_node = loaded[street].at(key);

            EXPECT_EQ(load_node.actions, orig_node.actions) << "Actions mismatch for " << key;
            EXPECT_EQ(load_node.regret_sum, orig_node.regret_sum) << "regret_sum mismatch for " << key;
            EXPECT_EQ(load_node.strat_sum, orig_node.strat_sum) << "strat_sum mismatch for " << key;
        }
    }
}

// save_nodes should reject data where any street has 0 infosets
TEST_F(SerializationTest, ThrowsOnEmptyStreet) {
    std::array<std::unordered_map<std::string, Node>, 4> nodes;
    nodes[0]["some_infoset"] = make_test_node({"c", "f"}, {1.0f, 2.0f}, {3.0f, 4.0f});
    // streets 1-3 are empty

    EXPECT_THROW(save_nodes(test_filepath, nodes), std::runtime_error);
}

// load_nodes should throw when the file doesn't exist
TEST_F(SerializationTest, ThrowsOnInvalidFilepath) {
    std::array<std::unordered_map<std::string, Node>, 4> nodes;
    EXPECT_THROW(load_nodes("nonexistent/path/weights.bin", nodes), std::runtime_error);
}

// Verify that very long infoset keys (500 chars) survive the round trip without truncation
TEST_F(SerializationTest, HandlesLargeActionStrings) {
    std::array<std::unordered_map<std::string, Node>, 4> original;

    std::string long_infoset(500, 'x');
    for (int street = 0; street < 4; street++) {
        original[street][long_infoset + std::to_string(street)] = make_test_node(
            {"c", "f"},
            {1.0f, 2.0f},
            {3.0f, 4.0f}
        );
    }

    save_nodes(test_filepath, original);

    std::array<std::unordered_map<std::string, Node>, 4> loaded;
    load_nodes(test_filepath, loaded);

    for (int street = 0; street < 4; street++) {
        std::string key = long_infoset + std::to_string(street);
        ASSERT_TRUE(loaded[street].count(key));
        EXPECT_EQ(loaded[street].at(key).regret_sum, original[street].at(key).regret_sum);
    }
}

// Stress test with 100 infosets per street (400 total) to verify nothing gets lost or mixed up
TEST_F(SerializationTest, MultipleInfosetsPerStreet) {
    std::array<std::unordered_map<std::string, Node>, 4> original;

    for (int street = 0; street < 4; street++) {
        for (int i = 0; i < 10000; i++) {
            std::string key = "infoset_" + std::to_string(street) + "_" + std::to_string(i);
            original[street][key] = make_test_node(
                {"c", "r", "f"},
                {static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)},
                {static_cast<float>(i + 1), static_cast<float>(i + 2), static_cast<float>(i + 3)}
            );
        }
    }

    save_nodes(test_filepath, original);

    std::array<std::unordered_map<std::string, Node>, 4> loaded;
    load_nodes(test_filepath, loaded);

    for (int street = 0; street < 4; street++) {
        ASSERT_EQ(loaded[street].size(), 10000u) << "Street " << street;
        for (const auto& [key, orig_node] : original[street]) {
            ASSERT_TRUE(loaded[street].count(key));
            EXPECT_EQ(loaded[street].at(key).regret_sum, orig_node.regret_sum);
            EXPECT_EQ(loaded[street].at(key).strat_sum, orig_node.strat_sum);
        }
    }
}
