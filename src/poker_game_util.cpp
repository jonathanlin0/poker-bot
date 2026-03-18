#include "../include/config.hpp"
#include "../include/poker_game_util.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

using std::array;
using std::max;
using std::runtime_error;
using std::vector;

uint8_t get_player_to_move(const vector<vector<Action>>& history) {
    uint8_t street = history.size() - 1;
    size_t num_actions = history.back().size();

    return num_actions % 2;
}

bool is_raise_valid(const vector<vector<Action>>& history, uint16_t raise_amount, uint16_t big_blind, uint16_t effective_stack) {
    const auto& current_round = history.back();
    
    // Calculate the amount to call (highest bet on this street)
    uint16_t amount_to_call = 0;
    if (!current_round.empty()) {
        amount_to_call = static_cast<uint16_t>(current_round.back().amount);
    }
    
    // Calculate the minimum raise (previous raise increment or big blind, whichever is larger)
    uint16_t min_raise = big_blind;
    if (current_round.size() >= 2) {
        uint16_t prev_raise = static_cast<uint16_t>(current_round.back().amount - current_round[current_round.size() - 2].amount);
        min_raise = max(min_raise, prev_raise);
    } else if (current_round.size() == 1) {
        min_raise = max(min_raise, static_cast<uint16_t>(current_round.back().amount));
    }
    
    // Check if raise amount meets minimum
    if (raise_amount < min_raise) {
        return false;
    }
    
    // Total bet = amount_to_call + raise_amount
    uint16_t total_bet = amount_to_call + raise_amount;
    
    // Check if player has enough chips
    if (total_bet > effective_stack) {
        return false;
    }
    
    return true;
}

bool is_game_over_from_history_vector(const vector<vector<Action>>& history) {
    if (history.empty()) return false;

    const auto& last_street = history.back();
    if (last_street.empty()) return false;

    // Someone folded
    if (last_street.back().type == 'f') return true;

    // All 4 streets complete and last river action is a call/check
    if (history.size() == 4 && last_street.size() >= 2 && last_street.back().type == 'c') {
        return true;
    }

    return false;
}

bool is_betting_street_done(const vector<Action>& street) {
    if (street.empty()) return false;

    // Someone folded
    if (street.back().type == 'f') return true;

    // Count non-blind actions
    size_t non_blind_actions = 0;
    for (const Action& a : street) {
        if (a.type != 'b') non_blind_actions++;
    }

    // Street is done when there are at least 2 non-blind actions and the last is a call/check
    if (non_blind_actions >= 2 && street.back().type == 'c') return true;

    return false;
}

PokerKit build_game(
    const array<array<Card, 2>, 2>& cards,
    const vector<vector<Action>>& all_history,
    const vector<Card>& board
) {
    PokerKit game(
        1,
        2,
        STARTING_STACK,
        STARTING_STACK,
        cards[0],
        cards[1]
    );

    for (size_t street = 0; street < all_history.size() && !game.is_game_over(); street++) {
        if (street == 1) {
            for (int i = 0; i < 3; i++) game.deal_board(board[i]);
        } else if (street == 2) {
            game.deal_board(board[3]);
        } else if (street == 3) {
            game.deal_board(board[4]);
        }

        // Replay actions for this street
        for (const Action& a : all_history[street]) {
            if (a.type == 'b') continue; // skip blind posts
            if (game.is_game_over()) break;
            switch (a.type) {
                case 'f': game.fold(); break;
                case 'c': game.check_or_call(); break;
                case 'r': {
                    // a.amount is raise_to (total bet), convert to increment for game.raise()
                    const auto& curr_round = game.get_bets().back();
                    uint16_t bet_to_face = (!curr_round.empty()) ? curr_round.back().amount : 0;
                    game.raise(a.amount - bet_to_face);
                    break;
                }
                case 'a': game.all_in(); break;
            }
        }
    }

    return game;
}

vector<Action> get_valid_actions(bool is_preflop, const PokerKit& game) {
    if (game.is_game_over()) {
        throw runtime_error("Cannot get valid actions: game is over");
    }

    vector<Action> actions;

    if (!game.get_can_still_bet()) {
        return actions;
    }

    uint8_t player = get_player_to_move(game.get_bets());
    auto stacks = game.get_stacks();
    uint16_t pot = game.get_pot_size();
    const auto& bets = game.get_bets();
    const auto& current_round = bets.back();

    // If facing an all-in, only fold or call
    if (!current_round.empty() && current_round.back().type == 'a') {
        actions.push_back(Action('f', -1));
        actions.push_back(Action('c', 0));
        return actions;
    }

    uint16_t player_current_bet = game.get_player_current_raise(player);
    uint16_t effective_stack = stacks[player] + player_current_bet;

    // Bet to face (opponent's last action amount)
    uint16_t bet_to_face = 0;
    if (!current_round.empty() && current_round.back().amount > 0) {
        bet_to_face = static_cast<uint16_t>(current_round.back().amount);
    }

    // Fold and Check/Call are always valid
    actions.push_back(Action('f', -1));
    actions.push_back(Action('c', 0));

    // Count raises in the current street
    int raises_this_street = 0;
    for (const auto& action : current_round) {
        if (action.type == 'r') raises_this_street++;
    }

    if (raises_this_street >= MAX_RAISES_PER_STREET) {
        return actions;
    }

    // Check each raise using is_raise_valid
    const uint16_t big_blind = 2;  // In SB units
    uint16_t call_cost = bet_to_face - player_current_bet;  // how much to call
    float effective_pot = static_cast<float>(pot + call_cost);

    if (is_preflop) {
        // Preflop: raise increments in BB
        for (uint8_t bb : PREFLOP_RAISE_SIZES) {
            uint16_t raise_increment = bb * 2;  // Convert BB to SB
            if (is_raise_valid(bets, raise_increment, big_blind, effective_stack)) {
                uint16_t raise_to = bet_to_face + raise_increment;
                if (raise_to < effective_stack) {  // Exclude all-in (handled separately)
                    actions.push_back(Action('r', raise_to));
                }
            }
        }
    } else {
        // Postflop: raise increments as pot fraction
        for (float mult : POSTFLOP_RAISE_SIZES) {
            uint16_t raise_increment = static_cast<uint16_t>(effective_pot * mult);
            if (is_raise_valid(bets, raise_increment, big_blind, effective_stack)) {
                uint16_t raise_to = bet_to_face + raise_increment;
                if (raise_to < effective_stack) {  // Exclude all-in (handled separately)
                    actions.push_back(Action('r', raise_to, mult));
                }
            }
        }
    }

    // All-in: always valid (unless facing all-in, handled above)
    actions.push_back(Action('a', effective_stack));

    return actions;
}