#ifndef POKER_GAME_UTIL_HPP
#define POKER_GAME_UTIL_HPP

#include <cstdint>
#include <vector>
#include "action.hpp"
#include "card.hpp"
#include "pokerkit.hpp"

// Returns which player's turn it is (0 = SB, 1 = BB) based on the betting history.
uint8_t get_player_to_move(const std::vector<std::vector<Action>>& history);

// Checks if a raise is valid given the betting history, raise amount, big blind, and player's effective stack.
// Returns true if the raise is valid, false otherwise.
bool is_raise_valid(const std::vector<std::vector<Action>>& history, uint16_t raise_amount, uint16_t big_blind, uint16_t effective_stack);

// Checks if the game is over based solely on the betting history vector.
// Game is over if:
//   1. Someone folded (last action on the last street is 'f')
//   2. All 4 streets are complete and the river betting is done
bool is_game_over_from_history_vector(const std::vector<std::vector<Action>>& history);

// Checks if a single betting street is done.
// A street is done if:
//   1. Someone folded
//   2. There are at least 2 non-blind actions and the last action is a call/check
bool is_betting_street_done(const std::vector<Action>& street);

// Builds a PokerKit game from cards, history, and board, replaying all actions.
PokerKit build_game(
    const std::array<std::array<Card, 2>, 2>& cards,
    const std::vector<std::vector<Action>>& all_history,
    const std::vector<Card>& board
);

// Returns the valid actions for the current game state.
// is_preflop: true for preflop, false for postflop (flop/turn/river)
// Uses is_raise_valid to check each raise.
std::vector<Action> get_valid_actions(bool is_preflop, const PokerKit& game);

#endif // POKER_GAME_UTIL_HPP
