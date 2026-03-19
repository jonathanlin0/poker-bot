#include "../include/pokerkit.hpp"
#include "../include/infoset_calculator.hpp"
#include "../include/card.hpp"
#include "../include/config.hpp"
#include "../include/util.hpp"
#include <array>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ==================== Globals ====================
std::atomic<bool> should_stop(false);
std::mutex print_mutex;

void signal_handler(int) {
    should_stop.store(true);
}

// [0] = wins (ties count as 0.5), [1] = total
using ResultMap = std::array<std::unordered_map<std::string, std::array<float, 2>>, 4>;

// Note: make sure to update this if the delimiter changes in infoset_calculator.cpp
const std::string BUCKET_DELIMITER = " ";

// ==================== File I/O ====================
ResultMap load_existing_data() {
    ResultMap data{};
    std::ifstream file(PRECOMPUTED_EQUITIES_FILE);
    if (!file.is_open()) return data;

    std::string infoset, wins_str, total_str;
    while (std::getline(file, infoset)) {
        if (!std::getline(file, wins_str) || !std::getline(file, total_str)) break;

        // Extract street from the infoset (second token: "player street ...")
        size_t first_delim = infoset.find(BUCKET_DELIMITER);
        size_t second_delim = infoset.find(BUCKET_DELIMITER, first_delim + 1);
        int street = std::stoi(infoset.substr(first_delim + 1, second_delim - first_delim - 1));

        data[street][infoset][0] += std::stof(wins_str);
        data[street][infoset][1] += std::stof(total_str);
    }
    return data;
}

void save_data(const ResultMap& data) {
    std::filesystem::create_directories("data");
    std::ofstream file(PRECOMPUTED_EQUITIES_FILE); // automatically gets destroyed when out of scope
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + PRECOMPUTED_EQUITIES_FILE);
    }
    for (int s = 0; s < 4; s++) {
        for (const auto& [key, stats] : data[s]) {
            file << key << "\n" << stats[0] << "\n" << stats[1] << "\n";
        }
    }
}

// ==================== Worker ====================
ResultMap simulate_equity(int thread_id) {
    ResultMap results{};
    // dummy actions used for infoset creation
    std::vector<Action> dummy_actions;

    for (int hand = 0; hand < PRECOMPUTE_EQUITY_HANDS_PER_THREAD; hand++) {
        if (should_stop.load()) break;

        if ((hand + 1) % PRECOMPUTE_EQUITY_PRINT_INTERVAL == 0) {
            std::lock_guard<std::mutex> lock(print_mutex); // lock the mutex to avoid race conditions. automatically unlocks when out of scope
            std::cout << "Thread " << thread_id << ": " << (hand + 1) << " hands played" << std::endl;
        }

        std::vector<Card> deck = get_new_deck(true);
        Card c0 = deck.back(); deck.pop_back();
        Card c1 = deck.back(); deck.pop_back();
        Card c2 = deck.back(); deck.pop_back();
        Card c3 = deck.back(); deck.pop_back();

        PokerKit game(1, 2, STARTING_STACK, STARTING_STACK, {c0, c1}, {c2, c3});

        std::string infosets[4][2]; // [street][player]

        // Preflop
        for (uint8_t p = 0; p < 2; p++) {
            infosets[0][p] = trim_aggression_and_actions_off_infoset(
                InfosetCalculator::get_infoset_from_game(game, p, dummy_actions));
        }
        game.check_or_call();
        game.check_or_call();

        // Flop
        for (int i = 0; i < 3; i++) { game.deal_board(deck.back()); deck.pop_back(); }
        for (uint8_t p = 0; p < 2; p++) {
            infosets[1][p] = trim_aggression_and_actions_off_infoset(
                InfosetCalculator::get_infoset_from_game(game, p, dummy_actions));
        }
        game.check_or_call();
        game.check_or_call();

        // Turn
        game.deal_board(deck.back()); deck.pop_back();
        for (uint8_t p = 0; p < 2; p++) {
            infosets[2][p] = trim_aggression_and_actions_off_infoset(
                InfosetCalculator::get_infoset_from_game(game, p, dummy_actions));
        }
        game.check_or_call();
        game.check_or_call();

        // River
        game.deal_board(deck.back()); deck.pop_back();
        for (uint8_t p = 0; p < 2; p++) {
            infosets[3][p] = trim_aggression_and_actions_off_infoset(
                InfosetCalculator::get_infoset_from_game(game, p, dummy_actions));
        }
        game.check_or_call();
        game.check_or_call(); // game ends here (river showdown)

        // Determine winner from stack deltas
        auto stacks = game.get_stacks();
        int8_t winner;
        if (stacks[0] > STARTING_STACK) winner = 0;
        else if (stacks[1] > STARTING_STACK) winner = 1;
        else winner = -1;

        for (int s = 0; s < 4; s++) {
            for (int p = 0; p < 2; p++) {
                auto& curr_data = results[s][infosets[s][p]];
                curr_data[1] += 1.0f;
                if (winner == p) curr_data[0] += 1.0f;
                else if (winner == -1) curr_data[0] += 0.5f;
            }
        }
    }

    return results;
}

// ==================== Main ====================
int main() {
    std::signal(SIGINT, signal_handler);

    std::cout << "Starting equity precomputation: "
              << PRECOMPUTE_EQUITY_NUM_THREADS << " threads x " << PRECOMPUTE_EQUITY_HANDS_PER_THREAD << " hands each"
              << std::endl;

    std::vector<ResultMap> thread_results(PRECOMPUTE_EQUITY_NUM_THREADS);
    std::vector<std::thread> threads;

    for (int i = 0; i < PRECOMPUTE_EQUITY_NUM_THREADS; i++) {
        // [&thread_results, i] allows the lambda function worker to access them
        auto worker = [&thread_results, i]() {
            thread_results[i] = simulate_equity(i);
        };
        threads.emplace_back(worker); // push back the thread object
    }

    // wait for the threads to finish
    for (auto& t : threads) {
        t.join();
    }

    if (should_stop.load()) {
        std::cout << "\nInterrupted by user." << std::endl;
    }

    const std::array<std::string, 4> street_names = {"Preflop", "Flop", "Turn", "River"};

    for (int i = 0; i < PRECOMPUTE_EQUITY_NUM_THREADS; i++) {
        std::cout << "\n--- Thread " << i << " ---" << std::endl;
        for (int s = 0; s < 4; s++) {
            std::cout << "  " << street_names[s] << ": "
                      << thread_results[i][s].size() << " unique infosets" << std::endl;
        }
    }

    // Merge all thread results
    ResultMap merged{};
    for (int i = 0; i < PRECOMPUTE_EQUITY_NUM_THREADS; i++) {
        for (int s = 0; s < 4; s++) {
            for (const auto& [key, stats] : thread_results[i][s]) {
                merged[s][key][0] += stats[0];
                merged[s][key][1] += stats[1];
            }
        }
    }

    // Load existing data and combine, tracking new infosets
    ResultMap existing = load_existing_data();
    std::array<size_t, 4> new_infosets{}; // number of new infosets for each street
    for (int s = 0; s < 4; s++) {
        for (const auto& [key, stats] : existing[s]) {
            merged[s][key][0] += stats[0];
            merged[s][key][1] += stats[1];
        }
        new_infosets[s] = merged[s].size() - existing[s].size();
    }

    save_data(merged);
    std::cout << "\nSaved to " << PRECOMPUTED_EQUITIES_FILE << std::endl;

    std::cout << "\n=== Merged Results (including previous runs) ===" << std::endl;
    for (int s = 0; s < 4; s++) {
        std::cout << street_names[s] << ": " << merged[s].size() << " unique infosets"
                  << " (" << new_infosets[s] << " new)" << std::endl;
    }

    return 0;
}
