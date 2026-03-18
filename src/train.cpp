#include "../include/pokerkit.hpp"
#include "../include/action.hpp"
#include "../include/card.hpp"
#include "../include/config.hpp"
#include "../include/node.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/infoset_calculator.hpp"
#include "../include/util.hpp"
#include "../include/validation.hpp"
#include <atomic>
#include <iostream>
#include <array>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>

using std::array;
using std::atomic;
using std::ceil;
using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::lock_guard;
using std::max;
using std::mutex;
using std::ofstream;
using std::pair;
using std::runtime_error;
using std::stoi;
using std::string;
using std::thread;
using std::unordered_map;
using std::vector;
namespace chrono = std::chrono;
namespace filesystem = std::filesystem;

int next_epoch_to_calculate_exploitability = 50000;

// global variables for stats
atomic<long long> total_hands_played = 0; // thread safe type
mutex cfr_mutex; // use via lock(cfr_mutex) for safe locking and unlocking. automatically unlocks when out of scope

int next_epoch_to_perform_validation = 1000;

// [street] -> { infoset_key -> Node }
array<unordered_map<string, Node>, 4> nodes;

unordered_map<string, float> infoset_to_hands_played;

EquityMap precomputed_equities;

double interval_regret_sum = 0.0;

int num_threads = 1;

/*
    Ensures that a node exists for the given infoset and street.
    Populates it with default values if it doesn't exist.
*/
void ensure_node_exists(uint8_t street, const string& infoset, const vector<Action>& valid_actions) {
    if (nodes[street].find(infoset) == nodes[street].end()) {
        nodes[street][infoset] = Node(valid_actions, precomputed_equities[street], infoset);
    }
}

// Returns strategy (normalized positive regrets) for an infoset
unordered_map<string, float> get_strat(uint8_t street, const string& infoset, const vector<Action>& valid_actions) {
    ensure_node_exists(street, infoset, valid_actions);
    const Node& node = nodes[street][infoset];
    unordered_map<string, float> strategy;

    // calculate total regret pre transformations
    float total_regret = 0.0f;
    for (size_t i = 0; i < node.actions.size(); i++) {
        total_regret += std::max(0.0f, node.regret_sum[i]);
    }

    // apply minimum regret sum for regularization purposes
    for (size_t i = 0; i < node.actions.size(); i++) {
        if (total_regret == 0.0f) {
            strategy[node.actions[i]] = 1.0f / valid_actions.size();
        } else {
            float uniform_prob = total_regret / valid_actions.size();
            float uniform_weight = max(0.0f, (15.0f - infoset_to_hands_played[infoset]) / 100.0f);
            strategy[node.actions[i]] = (uniform_prob * uniform_weight) + std::max(0.0f, node.regret_sum[i]); 
        }
    }

    // re-calculate total regret post transformations
    total_regret = 0.0f;
    for (const auto& [action, regret] : strategy) {
        total_regret += regret;
    }

    // normalize strategy
    for (const auto& [action, prob] : strategy) {
        strategy[action] /= total_regret;
    }
    
    return strategy;
}

void update_regret_sum(
    uint8_t street,
    const string& infoset,
    const unordered_map<string, float>& regret
) {
    Node& node = nodes[street][infoset];
    for (const auto& [action, val] : regret) {
        size_t idx = node.action_index(action);
        node.regret_sum[idx] = max(0.0f, node.regret_sum[idx] + val); // clamp the cum_regret to nonnegative for faster convergence. in vanilla CFR, we DON'T do this.
    }
}

void update_strat_sum(uint8_t street, const string& infoset, const unordered_map<string, float>& strat, const vector<Action>& valid_actions, int epoch) {
    // the first AVERAGING_DELAY epochs r treated as warm up, so they aren't considered for updating the strat sum
    int weight = max(0, epoch - AVERAGING_DELAY);
    if (weight == 0) return;
    ensure_node_exists(street, infoset, valid_actions);
    Node& node = nodes[street][infoset];
    for (const auto& [action, prob] : strat) {
        size_t idx = node.action_index(action);
        node.strat_sum[idx] += prob * weight; // iteration based weighting -> later iterations have more weight since they're considered more influential and representative of the optimal game strat
    }
}

void calculate_avg_strat() {
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
                // Uniform distribution if no samples
                float uniform_prob = 1.0f / node.actions.size();
                for (size_t i = 0; i < node.actions.size(); i++) {
                    node.avg_strat[i] = uniform_prob;
                }
            }
        }
    }
}

// Returns the payoff for each player given hole cards and full betting history
// Payoffs are in BB (starting stack delta / 2)
pair<float, float> get_regret(const PokerKit& game) {
    if (!game.is_game_over()) {
        throw runtime_error("Game is not over. " + game.debug_print_history());
    }

    auto stacks = game.get_stacks();
    return {
        (static_cast<int16_t>(stacks[0]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f,
        (static_cast<int16_t>(stacks[1]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f
    };
}

/*
    This is the main function that implements the External CFR algorithm.
    Returns the regret for the traversing player.
*/
float external_cfr(
    uint8_t traversing_player,
    const array<array<Card, 2>, 2>& cards,
    vector<vector<Action>> all_history,
    vector<Card> board,
    vector<Card> deck,
    int epoch
) {
    PokerKit game = build_game(cards, all_history, board);

    if (is_game_over_from_history_vector(all_history)) {
        pair<float, float> regret = get_regret(game);
        total_hands_played++;
        return traversing_player == 0 ? regret.first : regret.second;
    }

    if (is_betting_street_done(all_history.back())) {
        if (all_history.size() == 1) {
            board.push_back(deck.back());
            deck.pop_back();
            board.push_back(deck.back());
            deck.pop_back();
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
        }
        else if (all_history.size() == 2 || all_history.size() == 3) {
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
        }
        // don't have to consider when history size == 4, otherwise hand would be done

        // Rebuild game with the new board cards and street
        game = build_game(cards, all_history, board);
    }

    // Handle all-in runout: deal remaining cards when no more betting is possible
    // don't have to deal with preflop shove, cause previous if/else handles it and deals appropriately
    while (!game.is_game_over() && !game.get_can_still_bet()) {
        board.push_back(deck.back());
        deck.pop_back();
        all_history.push_back({});
        game = build_game(cards, all_history, board); // refresh game. TODO: make this more time efficient in the future
    }

    if (game.is_game_over()) {
        pair<float, float> regret = get_regret(game);
        total_hands_played++;
        return traversing_player == 0 ? regret.first : regret.second;
    }

    uint8_t player = get_player_to_move(all_history);

    // define the possible actions
    bool is_preflop = (all_history.size() == 1);
    vector<Action> valid_actions = get_valid_actions(is_preflop, game);

    uint8_t street = game.get_betting_street();

    string infoset = InfosetCalculator::get_infoset_from_game(game, player, valid_actions);


    // called up here since they're used by both traversing and sampled player
    unordered_map<string, float> strategy;
    {
        lock_guard<mutex> lock(cfr_mutex);
        ensure_node_exists(street, infoset, valid_actions);
        strategy = get_strat(street, infoset, valid_actions);
    }

    if (player == traversing_player) {
        unordered_map<string, float> action_util;
        for (const Action& a : valid_actions) {
            action_util[string(a)] = 0.0f;
        }

        float node_util = 0.0f;
        for (const Action& a : valid_actions) {
            all_history.back().push_back(a);

            action_util[string(a)] = external_cfr(traversing_player, cards, all_history, board, deck, epoch);
            node_util += strategy[string(a)] * action_util[string(a)];
            all_history.back().pop_back();
        }

        unordered_map<string, float> regret;
        for (const Action& a : valid_actions) {
            regret[string(a)] = action_util[string(a)] - node_util;
        }

        {
            lock_guard<mutex> lock(cfr_mutex);
            update_regret_sum(street, infoset, regret);

            // TEMP: track the positive clamped regret sum
            for (const auto& [action, val] : regret) {
                interval_regret_sum += max(0.0, static_cast<double>(val));
            }

            // TEMP
            // maybe in future have this tied to a separate mutex lock, not the same one as the nodes
            infoset_to_hands_played[infoset]++;
        }

        return node_util;
    }
    else { // acting player != traversing player
        vector<float> probs;
        for (const Action& a : valid_actions) {
            probs.push_back(strategy[string(a)]);
        }
        size_t idx = sample_from_distribution_list(probs);
        all_history.back().push_back(valid_actions[idx]);

        float util = external_cfr(
            traversing_player,
            cards,
            all_history,
            board,
            deck,
            epoch
        );

        {
            lock_guard<mutex> lock(cfr_mutex);
            update_strat_sum(street, infoset, strategy, valid_actions, epoch);
        }
        return util;
    }

    // should not get here    
    return -1; // will wrap around lol
}

void wrapper_cfr_iterations(const string& experiment_name) {
    precomputed_equities = load_precomputed_equities();

    // Clear and create data/<experiment_name>/ folder once at start
    string experiment_dir = "data/" + experiment_name;
    filesystem::remove_all(experiment_dir);
    filesystem::create_directories(experiment_dir);
    filesystem::create_directories(experiment_dir + "/variant_play");

    for (int i = 0; i < EPOCHS; i++) {
        if (i % VALIDATION_INTERVAL == 0) {
            cout << "Epoch " << i << " completed (" << total_hands_played << " hands played)" << endl;
            
            // TEMP: saves the change in regret sum for
            if (i > 0) {
                ofstream regret_file(experiment_dir + "/regret.txt", ios::app);
                if (!regret_file.is_open()) {
                    throw runtime_error("Failed to open file: " + experiment_dir + "/regret.txt");
                }
                regret_file << i << "\n" << interval_regret_sum << "\n";
                regret_file.close();
                // reset the interval regret sum
                interval_regret_sum = 0.0;
            }
        }

        if (i == next_epoch_to_calculate_exploitability) {
            cout << "Computing exploitability at epoch " << i << "..." << endl;
            auto exploit_start = chrono::steady_clock::now();
            float exploitability = Validation::compute_exploitability(nodes, precomputed_equities);
            double exploit_secs = chrono::duration<double>(chrono::steady_clock::now() - exploit_start).count();
            cout << "Exploitability: (" << exploit_secs << "s)" << endl;
            string exploit_path = experiment_dir + "/exploitability.txt";
            ofstream exploit_file(exploit_path, ios::app);
            if (!exploit_file.is_open()) {
                throw runtime_error("Failed to open file: " + exploit_path);
            }
            exploit_file << i << "\n" << exploitability << "\n";
            exploit_file.close();
            next_epoch_to_calculate_exploitability = ceil(next_epoch_to_calculate_exploitability * 1.5);
        }

        if (i == next_epoch_to_perform_validation) {
            cout << "Performing validation at epoch " << i << endl;
            calculate_avg_strat();
            Validation::play_variants(experiment_dir, i, VARIANT_NAMES, nodes, infoset_to_hands_played, precomputed_equities);
            next_epoch_to_perform_validation = ceil((next_epoch_to_perform_validation != 0 ? next_epoch_to_perform_validation : 1) * 1.3);
        }
        
        auto run_traversal = [&](uint8_t traversing_player) {
            // Create a new shuffled deck
            vector<Card> deck = get_new_deck(true);
            
            // Draw 2 cards for each player from the back of the deck
            Card c0 = deck.back(); deck.pop_back();
            Card c1 = deck.back(); deck.pop_back();
            Card c2 = deck.back(); deck.pop_back();
            Card c3 = deck.back(); deck.pop_back();
            array<array<Card, 2>, 2> cards = {{{c0, c1}, {c2, c3}}};
            
            // Initialize empty history (start with one empty street for preflop)
            vector<vector<Action>> all_history = {{}};
            
            // Initialize empty board
            vector<Card> board;
            
            external_cfr(traversing_player, cards, all_history, board, deck, i);
        };

        vector<thread> threads;
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back(run_traversal, 0);
            threads.emplace_back(run_traversal, 1);
        }
        for (auto& th : threads) {
            th.join();
        }
    }
}

int main(int argc, char* argv[]) {
    string experiment_name = "default";

    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-n" && i + 1 < argc) {
            experiment_name = argv[++i];
        } else if (string(argv[i]) == "--num-threads" && i + 1 < argc) {
            num_threads = stoi(argv[++i]);
            if (num_threads < 1) {
                cerr << "Error: --num-threads must be a positive integer" << endl;
                return 1;
            }
        } else {
            cerr << "Usage: " << argv[0] << " -n <name> [--num-threads <N>]" << endl;
            return 1;
        }
    }

    wrapper_cfr_iterations(experiment_name);

    cout << "Node keys per street:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "  Street " << i << ": " << nodes[i].size() << " infosets" << endl;
    }

    return 0;
}
