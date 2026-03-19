#include "../include/validation.hpp"
#include "../include/pokerkit.hpp"
#include "../include/action.hpp"
#include "../include/card.hpp"
#include "../include/config.hpp"
#include "../include/initial_strategy_getter.hpp"
#include "../include/node.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/infoset_calculator.hpp"
#include "../include/util.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

using std::array;
using std::cout;
using std::cref;
using std::default_random_engine;
using std::endl;
using std::ios;
using std::max;
using std::ofstream;
using std::random_device;
using std::ref;
using std::remove;
using std::runtime_error;
using std::shuffle;
using std::string;
using std::thread;
using std::to_string;
using std::unordered_map;
using std::vector;
namespace filesystem = std::filesystem;

void apply_action_to_game(PokerKit& game, const Action& a) {
    switch (a.type) {
        case 'f': game.fold(); break;
        case 'c': game.check_or_call(); break;
        case 'r': {
            const auto& curr_round = game.get_bets().back();
            uint16_t bet_to_face = (!curr_round.empty()) ? curr_round.back().amount : 0;
            game.raise(a.amount - bet_to_face);
            break;
        }
        case 'a': game.all_in(); break;
    }
}

// Rollout simulation where everyone just calls/checks until game ends
// Opponent's hand is randomly dealt from remaining deck
float simulate_rollout(
    const array<Card, 2>& bot_hand,
    vector<vector<Action>> all_history,
    vector<Card> board,
    vector<Card> deck,
    uint8_t bot_position
) {
    // Shuffle deck and deal random opponent hand
    shuffle(deck.begin(), deck.end(), default_random_engine(random_device{}()));
    Card opp0 = deck.back(); deck.pop_back();
    Card opp1 = deck.back(); deck.pop_back();
    
    // Build cards array with bot in correct position
    array<array<Card, 2>, 2> cards = (bot_position == 0)
        ? array<array<Card, 2>, 2>{{{bot_hand[0], bot_hand[1]}, {opp0, opp1}}}
        : array<array<Card, 2>, 2>{{{opp0, opp1}, {bot_hand[0], bot_hand[1]}}};
    
    while (true) {
        PokerKit game = build_game(cards, all_history, board);

        // Check if game ended (fold or river call)
        if (is_game_over_from_history_vector(all_history)) {
            auto stacks = game.get_stacks();
            return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        }

        // Advance to next street if current betting round is complete
        if (is_betting_street_done(all_history.back())) {
            if (all_history.size() == 1) {
                // Deal flop
                board.push_back(deck.back()); deck.pop_back();
                board.push_back(deck.back()); deck.pop_back();
                board.push_back(deck.back()); deck.pop_back();
                all_history.push_back({});
            } else if (all_history.size() == 2 || all_history.size() == 3) {
                // Deal turn or river
                board.push_back(deck.back()); deck.pop_back();
                all_history.push_back({});
            } else if (all_history.size() == 4) {
                // Showdown after river
                game = build_game(cards, all_history, board);
                auto stacks = game.get_stacks();
                return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
            }
            game = build_game(cards, all_history, board);
        }

        // Handle all-in runout
        // don't have to deal with preflop shove, cause previous if/else handles it and deals appropriately
        while (!game.is_game_over() && !game.get_can_still_bet()) {
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
            game = build_game(cards, all_history, board);
        }

        if (game.is_game_over()) {
            auto stacks = game.get_stacks();
            return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        }

        bool is_preflop = (all_history.size() == 1);
        vector<Action> valid_actions = get_valid_actions(is_preflop, game);

        if (valid_actions.empty()) {
            throw runtime_error("Valid actions is empty in simulate_rollout");
        }

        // Always pick call/check action
        Action chosen_action = valid_actions[0];  // fallback
        for (const Action& a : valid_actions) {
            if (a.type == 'c') {
                chosen_action = a;
                break;
            }
        }

        all_history.back().push_back(chosen_action);
    }
}

float get_win_rate_for_tight_agro(
    const unordered_map<string, float>& base_avg_strat,
    const array<Card, 2>& player_hand,
    uint8_t player_position,
    const vector<vector<Action>>& all_history,
    const vector<Card>& board
) {
    unordered_map<string, float> result = base_avg_strat;
    
    // For each action, simulate TIGHT_AGRO_NUM_SIMS hands from this state
    int wins = 0;
    
    for (int i = 0; i < TIGHT_AGRO_NUM_SIMS; i++) {
        // Create a fresh deck excluding known cards (player's hand + board)
        vector<Card> deck = get_new_deck(true);
        
        auto remove_card = [&deck](const Card& c) {
            deck.erase(remove(deck.begin(), deck.end(), c), deck.end());
        };
        remove_card(player_hand[0]);
        remove_card(player_hand[1]);
        for (const Card& c : board) {
            remove_card(c);
        }
        
        // Copy history and append this action
        vector<vector<Action>> sim_history = all_history;
        
        // Simulate rollout - opponent hand dealt randomly inside
        float ev = simulate_rollout(player_hand, sim_history, board, deck, player_position);
        if (ev >= 0) {
            wins++;
        }
    }
    float win_rate = static_cast<float>(wins) / TIGHT_AGRO_NUM_SIMS;
    
    
    // can append the win rate here to some external file for plotting if you want to later
    
    return win_rate;
}

unordered_map<string, float> get_avg_strat_variant(
    const unordered_map<string, float>& base_avg_strat,
    const string& variant_name,
    const array<Card, 2>& player_hand,
    uint8_t player_position,
    const vector<vector<Action>>& all_history,
    const vector<Card>& board,
    const string& infoset,
    uint8_t street,
    const EquityMap& equities
) {
    unordered_map<string, float> variant = base_avg_strat;

    if (variant_name == "tight-agro") {
        float win_rate = get_win_rate_for_tight_agro(base_avg_strat, player_hand, player_position, all_history, board);
        float threshold = board.size() == 1 ? 0.4f : 0.5f; // bimodal distribution of win rates, with it split with the following threshold (0.4 preflop and 0.5 postflop)
        if (win_rate < threshold) {
            return get_avg_strat_variant(base_avg_strat, "tight", player_hand, player_position, all_history, board, infoset, street, equities);
        } else {
            return get_avg_strat_variant(base_avg_strat, "agro", player_hand, player_position, all_history, board, infoset, street, equities);
        }
    } else if (variant_name == "initial-weights") {
        // TODO remove this repeated logic somewhere else, since node.cpp also does this trimming stuff
        float uniform = 1.0f / variant.size();
        vector<string> keys;
        vector<float> probs;
        vector<Action> actions;
        for (const auto& [action, prob] : variant) {
            keys.push_back(action);
            probs.push_back(uniform);
            actions.push_back(Action(action[0], -1)); // pass in -1 cause raise amount doesn't matter for initial weights. if initial weights r adjusted for raise amt later, then finish the constructor for Action based on string input
        }

        vector<float> initial_strategy = InitialStrategyGetter::get_initial_strategy(street, infoset, actions);

        for (size_t i = 0; i < keys.size(); i++) {
            variant[keys[i]] = initial_strategy[i];
        }
    } else if (variant_name == "over-call") {
        for (const auto& [action, prob] : variant) {
            if (action[0] == 'c') {
                variant[action] = std::max(60.0f, variant[action] * 3.0f);
            }
        }
    } else if (variant_name == "agro") {
        // get total number of raises / all-ins
        int total_raises = 0;
        for (const auto& [action, prob] : variant) {
            if (action[0] == 'r' || action[0] == 'a') {
                total_raises++;
            }
        }
        for (const auto& [action, prob] : variant) {
            if (action[0] == 'r' || action[0] == 'a') {
                variant[action] = std::max(75.0f / total_raises, variant[action] * 3.0f); // ~75% chance of raising distributed across the aggressive actions
            }
        }
    } else if (variant_name == "tight") {
        for (const auto& [action, prob] : variant) {
            if (action[0] == 'f') {
                variant[action] = std::max(50.0f, variant[action] * 4.0f);
            } else if (action[0] == 'c') {
                variant[action] = std::max(40.0f, variant[action] * 2.0f);
            }
            else if (action[0] == 'r' || action[0] == 'a') {
                variant[action] *= 0.3f;
            }
        }
    }
    else if (variant_name == "unif") { // completley uniform for each action. essentially the initial weights
        for (const auto& [action, prob] : variant) {
            variant[action] = 1.0f;
        }
    }
    else if (variant_name == "itself") {
        return variant;
    }
    else {
        throw runtime_error("Invalid variant name: " + variant_name);
    }

    // Renormalize
    float total = 0.0f;
    for (const auto& [action, prob] : variant) {
        total += prob;
    }
    assert (total > 0 && "Total probability is 0");
    if (total > 0) {
        for (const auto& [action, prob] : variant) {
            variant[action] /= total;
        }
    }

    return variant;
}

// Simulates a single hand between the original avg_strat and a variant.
// bot_position: 0 = SB, 1 = BB
// Returns PnL for the bot (original player) in BB.
// infosets_played is populated with infosets encountered during the hand.
float simulate_hand(
    uint8_t bot_position,
    const string& variant_name,
    const array<unordered_map<string, Node>, 4>& nodes,
    const EquityMap& equities
) {
    vector<Card> deck = get_new_deck(true);

    // Deal hole cards
    Card c0 = deck.back(); deck.pop_back();
    Card c1 = deck.back(); deck.pop_back();
    Card c2 = deck.back(); deck.pop_back();
    Card c3 = deck.back(); deck.pop_back();
    array<array<Card, 2>, 2> cards = {{{c0, c1}, {c2, c3}}};

    vector<vector<Action>> all_history = {{}};
    vector<Card> board;

    while (true) {
        PokerKit game = build_game(cards, all_history, board);

        // Check if game ended (fold or river call)
        if (is_game_over_from_history_vector(all_history)) {
            auto stacks = game.get_stacks();
            return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        }

        // Advance to next street if current betting round is complete
        if (is_betting_street_done(all_history.back())) {
            if (all_history.size() == 1) {
                // Deal flop
                board.push_back(deck.back()); deck.pop_back();
                board.push_back(deck.back()); deck.pop_back();
                board.push_back(deck.back()); deck.pop_back();
                all_history.push_back({});
            } else if (all_history.size() == 2 || all_history.size() == 3) {
                // Deal turn or river
                board.push_back(deck.back()); deck.pop_back();
                all_history.push_back({});
            } else if (all_history.size() == 4) {
                // Showdown after river
                game = build_game(cards, all_history, board);
                auto stacks = game.get_stacks();
                return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
            }
            game = build_game(cards, all_history, board);
        }

        // Handle all-in runout: deal remaining board cards when no more betting is possible
        while (!game.is_game_over() && !game.get_can_still_bet()) {
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
            game = build_game(cards, all_history, board);
        }

        if (game.is_game_over()) {
            auto stacks = game.get_stacks();
            return (static_cast<int16_t>(stacks[bot_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        }

        uint8_t player = get_player_to_move(all_history);
        bool is_preflop = (all_history.size() == 1);
        vector<Action> valid_actions = get_valid_actions(is_preflop, game);

        if (valid_actions.empty()) {
            throw runtime_error("Valid actions is empty in simulate_hand");
        }

        uint8_t street = game.get_betting_street();
        string infoset = InfosetCalculator::get_infoset_from_game(game, player, valid_actions);

        // Look up base strategy (avg_strat or uniform fallback)
        unordered_map<string, float> strategy;
        if (nodes[street].count(infoset)) {
            strategy = nodes[street].at(infoset).get_avg_strat_map();
        } else {
            // Infoset not seen during training — use uniform
            float uniform = 1.0f / valid_actions.size();
            for (const Action& a : valid_actions) {
                strategy[string(a)] = uniform;
            }
        }

        // Apply variant modification for the opponent
        if (player != bot_position) {
            strategy = get_avg_strat_variant(strategy, variant_name, cards[player], player, all_history, board, infoset, street, equities);
        }

        vector<float> probs;
        for (const Action& a : valid_actions) {
            probs.push_back(strategy[string(a)]);
        }
        size_t idx = sample_from_distribution_list(probs);
        all_history.back().push_back(valid_actions[idx]);
    }

    throw runtime_error("Should not reach here in simulate_hand");
    return 0.0f; // Should not reach here
}

/*
pseudo code for this function that calculates exploitability:

deal cards to both players, prepare full deck/board

def best_response(history, board, deck):
    if game_over:
        return payoff for exploiter

    if need_new_street:
        deal community cards from deck
    
    if current_player == bot:
        # bot plays fixed strategy -- weight ALL actions by probability
        strategy = lookup avg_strat for bot's infoset
        ev = 0
        for each action:
            ev += strategy[action] * best_response(history + action, ...)
        return ev

    if current_player == exploiter:
        # exploiter knows its own cards, so it computes its infoset,
        # but it does NOT sample -- it tries ALL actions and takes max
        best_ev = -infinity
        for each action:
            v = best_response(history + action, ...)
            best_ev = max(best_ev, v)
        return best_ev
*/

float best_response_value(
    uint8_t exploiter_position,
    const array<array<Card, 2>, 2>& cards,
    vector<vector<Action>> all_history,
    vector<Card> board,
    vector<Card> deck,
    PokerKit& game, // IMPORTANT: this is pass by reference, so ensure that the game object is copied before recursion (if needed)
    const array<unordered_map<string, Node>, 4>& nodes,
    const EquityMap& equities
) {
    if (is_game_over_from_history_vector(all_history)) {
        auto stacks = game.get_stacks();
        return (static_cast<int16_t>(stacks[exploiter_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
    }

    if (is_betting_street_done(all_history.back())) {
        if (all_history.size() == 1) {
            for (int i = 0; i < 3; i++) {
                Card c = deck.back(); deck.pop_back();
                board.push_back(c);
                game.deal_board(c);
            }
            all_history.push_back({});
        } else if (all_history.size() == 2 || all_history.size() == 3) {
            Card c = deck.back(); deck.pop_back();
            board.push_back(c);
            game.deal_board(c);
            all_history.push_back({});
        } else if (all_history.size() == 4) { // unreachable code. TODO: remove this elif branch
            auto stacks = game.get_stacks();
            return (static_cast<int16_t>(stacks[exploiter_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        }
    }

    // Handle all-in runout: deal remaining board cards when no more betting is possible
    while (!game.is_game_over() && !game.get_can_still_bet()) {
        Card c = deck.back(); deck.pop_back();
        board.push_back(c);
        game.deal_board(c);
        all_history.push_back({});
    }

    if (game.is_game_over()) {
        auto stacks = game.get_stacks();
        return (static_cast<int16_t>(stacks[exploiter_position]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
    }

    uint8_t player = get_player_to_move(all_history);
    bool is_preflop = (all_history.size() == 1);
    vector<Action> valid_actions = get_valid_actions(is_preflop, game);

    if (valid_actions.empty()) {
        throw runtime_error("Valid actions is empty in best_response_value");
    }

    uint8_t street = game.get_betting_street();
    string infoset = InfosetCalculator::get_infoset_from_game(game, player, valid_actions);

    unordered_map<string, float> strategy;
    if (nodes[street].count(infoset)) {
        strategy = nodes[street].at(infoset).get_avg_strat_map();
    } else {
        vector<float> initial_strategy = InitialStrategyGetter::get_initial_strategy(street, infoset, valid_actions);
        for (size_t i = 0; i < valid_actions.size(); i++) {
            strategy[string(valid_actions[i])] = initial_strategy[i];
        }
    }

    if (player == exploiter_position) {
        float best_ev = -1e9f;
        for (const Action& a : valid_actions) {
            PokerKit child_game = game;
            apply_action_to_game(child_game, a);
            all_history.back().push_back(a);
            float ev = best_response_value(exploiter_position, cards, all_history, board, deck, child_game, nodes, equities);
            best_ev = max(best_ev, ev);
            all_history.back().pop_back();
        }
        return best_ev;
    } else {
        float weighted_ev = 0.0f;
        for (const Action& a : valid_actions) {
            float prob = strategy[string(a)];
            if (prob <= 0.0f) continue;
            PokerKit child_game = game;
            apply_action_to_game(child_game, a);
            all_history.back().push_back(a);
            weighted_ev += prob * best_response_value(exploiter_position, cards, all_history, board, deck, child_game, nodes, equities);
            all_history.back().pop_back();
        }
        return weighted_ev;
    }
}

// updates the value of parameter result (passed by reference)
// done cause threads can't return values
void compute_exploitability_thread(
    uint8_t exploiter_pos,
    const array<unordered_map<string, Node>, 4>& nodes,
    const EquityMap& equities,
    float& result
) {
    vector<Card> deck = get_new_deck(true);

    Card c0 = deck.back(); deck.pop_back();
    Card c1 = deck.back(); deck.pop_back();
    Card c2 = deck.back(); deck.pop_back();
    Card c3 = deck.back(); deck.pop_back();
    array<array<Card, 2>, 2> cards = {{{c0, c1}, {c2, c3}}};

    vector<vector<Action>> all_history = {{}};
    vector<Card> board;

    PokerKit game = build_game(cards, all_history, board);
    result = best_response_value(exploiter_pos, cards, all_history, board, deck, game, nodes, equities);
}

float Validation::compute_exploitability(
    const array<unordered_map<string, Node>, 4>& nodes,
    const EquityMap& equities
) {
    float total = 0.0f;
    int num_batches = EXPLOITABILITY_NUM_SAMPLES / NUM_EXPLOITABILITY_THREADS;

    for (int batch = 0; batch < num_batches; batch++) {
        vector<thread> threads;
        vector<float> results(NUM_EXPLOITABILITY_THREADS, 0.0f);

        for (int t = 0; t < NUM_EXPLOITABILITY_THREADS; t++) {
            uint8_t exploiter_pos = (batch * NUM_EXPLOITABILITY_THREADS + t) % 2;
            threads.emplace_back(
                compute_exploitability_thread,
                exploiter_pos,
                cref(nodes), // cref is const reference
                cref(equities),
                ref(results[t])
            );
        }

        for (auto& th : threads) {
            th.join();
        }

        for (float r : results) {
            total += r;
        }
    }

    return total / (num_batches * NUM_EXPLOITABILITY_THREADS);
}

void Validation::play_variants(
    const string& experiment_dir,
    int epoch,
    const vector<string>& variant_names,
    const array<unordered_map<string, Node>, 4>& nodes,
    const unordered_map<string, float>& infoset_to_hands_played,
    const EquityMap& equities
) {
    for (const string& variant_name : variant_names) {
        string csv_path = experiment_dir + "/variant_play/" + variant_name + "_play_data.csv";
        
        // Create file for variant play data if it doesn't exist
        if (!filesystem::exists(csv_path)) {
            ofstream create_file(csv_path);
            create_file.close();
        }

        float total_pnl = 0.0f;
        string itself_results;

        for (int i = 0; i < HANDS_PER_VARIANT; i++) {
            float curr_pnl = simulate_hand(i % 2, variant_name, nodes, equities);

            if (variant_name == "itself" && i % 3 == 0) { // skip 2/3 so txt file isn't overflowed
                itself_results += to_string(curr_pnl) + "\n";
            }

            total_pnl += curr_pnl;
        }

        // TEMP: save the results of the bot playing itself to a file to be plotted
        if (variant_name == "itself") {
            string itself_path = experiment_dir + "/itself_hand_results.txt";
            ofstream itself_file(itself_path, ios::app);
            if (!itself_file.is_open()) {
                throw runtime_error("Failed to open file: " + itself_path);
            }
            itself_file << itself_results;
            itself_file.close();
        }
        

        // Append epoch and total_pnl to CSV
        ofstream csv_file(csv_path, ios::app);
        if (!csv_file.is_open()) {
            throw runtime_error("Failed to open file: " + csv_path);
        }
        csv_file << epoch << "," << total_pnl << "\n";
        csv_file.close();
    }

    // TEMP: saving the number of infosets at each street at the current epoch
    string infoset_path = experiment_dir + "/infosets_wrt_epoch.txt";
    ofstream infoset_file(infoset_path, ios::app);
    if (!infoset_file.is_open()) {
        throw runtime_error("Failed to open file: " + infoset_path);
    }
    infoset_file << epoch << "\n";
    for (int i = 0; i < 4; i++) {
        infoset_file << nodes[i].size() << "\n";
    }
    infoset_file << "\n";
    infoset_file.close();
}

