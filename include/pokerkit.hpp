#ifndef POKERKIT_HPP
#define POKERKIT_HPP

#include <cstdint>
#include <array>
#include <vector>
#include "card.hpp"
#include "action.hpp"

// Will have to create a new instance of this class for each hand
class PokerKit {
public:
    // Constructors
    PokerKit(uint16_t small_blind, uint16_t big_blind, uint16_t stack_p0, uint16_t stack_p1);
    PokerKit(uint16_t small_blind, uint16_t big_blind, uint16_t stack_p0, uint16_t stack_p1,
             std::array<Card, 2> sb_hand, std::array<Card, 2> bb_hand);

    // Getters
    std::array<uint16_t, 2> get_stacks() const;
    uint16_t get_pot_size() const;
    uint8_t get_betting_street() const;
    bool is_game_over() const;
    bool get_can_still_bet() const;
    const std::vector<Card>& get_board() const;
    const std::vector<Card>& get_hand(uint8_t player) const;
    const std::vector<std::vector<Action>>& get_bets() const;
    // Get the amount a player has bet for the current betting street
    uint16_t get_player_current_raise(uint8_t player) const;
    std::string debug_print_history() const;  // Returns betting history as string for debugging
    // Returns the last action on the current betting street, or Action('x', 0) if no actions yet.
    Action get_last_action() const;

    // Actions
    void check_or_call();
    void fold();
    // Amount is the raise amount (excluding the amount to call). So it's total amount bet for that round minus the amount to call.
    void raise(uint16_t amount);
    void all_in();
    void deal_board(Card card);
    void deal_board();

private:
    uint16_t small_blind;    // Small blind size
    uint16_t big_blind;      // Big blind size
    std::array<uint16_t, 2> initial_stacks;  // Initial stacks at the start of the hand
    std::array<uint16_t, 2> stacks;  // Starting stacks for each player. 0 is SB, 1 is BB
    std::vector<Card> deck;  // The deck of cards
    std::vector<std::vector<Card>> hands = {{}, {}};  // Hands for each player (0 = SB, 1 = BB)
    std::vector<Card> board;  // Community cards on the board
    uint16_t pot = 0;  // Current pot size
    std::vector<std::vector<Action>> bets = {};  // Actions for each player
    bool game_over = false;  // Whether the game has ended,. private so other classes can't change it
    bool can_still_bet = true;  // False if game is over or someone went all in and other person called

    void deal_hands(std::array<Card, 2> small_blind_hand, std::array<Card, 2> big_blind_hand);
    void deal_hands();
    void start_preflop();
    
    int8_t calculate_winner(); // returns -1 for tie, 0 for SB win, 1 for BB win
    void settle_pot();
};

#endif // POKERKIT_HPP

