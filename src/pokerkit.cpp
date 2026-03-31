#include "../include/pokerkit.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/util.hpp"
#include <stdexcept>
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <tuple>
#include <cassert>
#include <cmath>

PokerKit::PokerKit(uint16_t small_blind, uint16_t big_blind, uint16_t stack_p0, uint16_t stack_p1)
    : small_blind(small_blind),
      big_blind(big_blind),
      initial_stacks{stack_p0, stack_p1},
      stacks{stack_p0, stack_p1},
      deck{get_new_deck()} {
    deal_hands();
    start_preflop();
}

PokerKit::PokerKit(uint16_t small_blind, uint16_t big_blind, uint16_t stack_p0, uint16_t stack_p1,
                   std::array<Card, 2> sb_hand, std::array<Card, 2> bb_hand)
    : small_blind(small_blind),
      big_blind(big_blind),
      initial_stacks{stack_p0, stack_p1},
      stacks{stack_p0, stack_p1},
      deck{get_new_deck()} {
    deal_hands(sb_hand, bb_hand);
    start_preflop();
}

std::array<uint16_t, 2> PokerKit::get_stacks() const {
    return stacks;
}

uint16_t PokerKit::get_pot_size() const {
    return pot;
}

uint8_t PokerKit::get_betting_street() const {
    return static_cast<uint8_t>(bets.size() - 1);
}

bool PokerKit::is_game_over() const {
    return game_over;
}

bool PokerKit::get_can_still_bet() const {
    return can_still_bet;
}

const std::vector<Card>& PokerKit::get_board() const {
    return board;
}

const std::vector<Card>& PokerKit::get_hand(uint8_t player) const {
    return hands[player];
}

const std::vector<std::vector<Action>>& PokerKit::get_bets() const {
    return bets;
}

void PokerKit::deal_hands(std::array<Card, 2> small_blind_hand, std::array<Card, 2> big_blind_hand) {
    // Check all 4 cards exist in deck and remove them
    std::array<Card, 4> all_cards{small_blind_hand[0], small_blind_hand[1], big_blind_hand[0], big_blind_hand[1]};
    
    for (const Card& card : all_cards) {
        auto it = std::find_if(deck.begin(), deck.end(), [&card](const Card& deck_card) {
            return deck_card.suit == card.suit && deck_card.rank == card.rank;
        });
        if (it == deck.end()) {
            throw std::runtime_error("Card not found in deck");
        }
        deck.erase(it);
    }
    hands[0].push_back(small_blind_hand[0]);
    hands[0].push_back(small_blind_hand[1]);
    hands[1].push_back(big_blind_hand[0]);
    hands[1].push_back(big_blind_hand[1]);
}

void PokerKit::deal_hands() {
    if (deck.size() < 4) {
        throw std::runtime_error("Not enough cards in deck to deal hands");
    }
    std::array<Card, 2> small_blind_hand{deck[deck.size() - 1], deck[deck.size() - 2]};
    std::array<Card, 2> big_blind_hand{deck[deck.size() - 3], deck[deck.size() - 4]};
    deal_hands(small_blind_hand, big_blind_hand);
}

void PokerKit::start_preflop() {
    stacks[0] -= small_blind;
    stacks[1] -= big_blind;
    std::vector<Action> preflop_bets = {};
    preflop_bets.push_back(Action('b', small_blind));
    preflop_bets.push_back(Action('b', big_blind));
    bets.push_back(preflop_bets);
    pot += small_blind + big_blind;
}


Action PokerKit::get_last_action() const {
    const auto& current_round = bets.back();
    if (current_round.empty()) {
        return Action('x', 0);  // No action yet
    }
    return current_round.back();
}

uint16_t PokerKit::get_player_current_raise(uint8_t player) const {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    const auto& current_round = bets.back();
    // Find the last action at an index matching player's parity (even for SB, odd for BB)
    for (int i = static_cast<int>(current_round.size()) - 1; i >= 0; --i) {
        if (i % 2 == player) {
            return current_round[i].amount;
        }
    }
    return 0;
}

void PokerKit::check_or_call() {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    if (!can_still_bet) {
        throw std::runtime_error("Can't bet anymore because someone went all in and the other person called");
    }
    
    uint8_t player = get_player_to_move(bets);
    auto& current_round = bets.back();
    if (current_round.empty()) {
        // Check
        current_round.push_back(Action('c', 0));
    } else {
        // Call - match the last bet amount
        uint16_t current_raise = get_player_current_raise(player);
        uint16_t call_amount = current_round.back().amount;
        uint16_t additional_chips = call_amount - current_raise;  // new chips going into pot
        stacks[player] += current_raise; // add back prev bet
        stacks[player] -= call_amount;
        pot += additional_chips;
        current_round.push_back(Action('c', call_amount));

        // If calling an all-in, no more betting can occur
        if (current_round.size() >= 2 && current_round[current_round.size() - 2].type == 'a') {
            can_still_bet = false;
        }
    }

    // End game on river (street 4) after both players have acted
    if (bets.size() == 4 && current_round.size() >= 2) {
        game_over = true;
        can_still_bet = false;
        settle_pot();
    }
}

void PokerKit::fold() {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    if (!can_still_bet) {
        throw std::runtime_error("Can't bet anymore because someone went all in and the other person called");
    }
    bets.back().push_back(Action('f', -1));
    game_over = true;
    can_still_bet = false;
    settle_pot();
}

void PokerKit::raise(uint16_t raise_amount) {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    if (!can_still_bet) {
        throw std::runtime_error("Can't bet anymore because someone went all in and the other person called");
    }
    if (!bets.back().empty() && bets.back().back().type == 'a') {
        throw std::runtime_error("Can't raise after opponent went all in, can only call");
    }
    
    uint8_t player = get_player_to_move(bets);
    uint16_t player_current_bet = get_player_current_raise(player);
    uint16_t effective_stack = stacks[player] + player_current_bet;
    
    // Check if the raise is valid
    if (!is_raise_valid(bets, raise_amount, big_blind, effective_stack)) {
        throw std::runtime_error("Invalid raise amount. " + debug_print_history() + "| Attempted raise: " + std::to_string(raise_amount));
    }
    
    // Calculate the amount to call (highest bet on this street)
    uint16_t amount_to_call = 0;
    const auto& current_round = bets.back();
    if (!current_round.empty()) {
        amount_to_call = current_round.back().amount;
    }
    
    // Total bet = amount_to_call + raise_amount
    uint16_t total_bet = amount_to_call + raise_amount;
    
    uint16_t call_cost = amount_to_call - player_current_bet;  // how much to call
    float effective_pot = static_cast<float>(pot + call_cost);
    float pot_mult = std::round(static_cast<float>(raise_amount) / effective_pot * 100.0f) / 100.0f; // effective pot should never be 0, due to blinds

    uint16_t additional_chips = total_bet - player_current_bet;  // new chips going into pot
    stacks[player] += player_current_bet; // add back prev bet
    stacks[player] -= total_bet;
    pot += additional_chips;
    bets.back().push_back(Action('r', total_bet, pot_mult));
}

void PokerKit::all_in() {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    if (!can_still_bet) {
        throw std::runtime_error("Can't bet anymore because someone went all in and the other person called");
    }
    if (!bets.back().empty() && bets.back().back().type == 'a') {
        throw std::runtime_error("Can't go all in after opponent went all in, can only call");
    }
    
    uint8_t player = get_player_to_move(bets);
    uint16_t current_raise = get_player_current_raise(player);
    stacks[player] += current_raise;  // add back prev bet
    uint16_t all_in_amount = stacks[player];
    uint16_t additional_chips = all_in_amount - current_raise;  // new chips going into pot
    stacks[player] = 0;
    pot += additional_chips;
    bets.back().push_back(Action('a', all_in_amount));
}

int8_t PokerKit::calculate_winner() {
    /*
    Note: this function converts ranks to indic``es for easier comparisons
    */
    if (!game_over) {
        throw std::runtime_error("Game is not over");
    }
    
    // Check for fold - if someone folded, the other player wins
    const auto& last_round = bets.back();
    if (!last_round.empty() && last_round.back().type == 'f') {
        int folding_player = (last_round.size() - 1) % 2;
        return (folding_player == 0) ? 1 : 0;  // Return the other player as winner
    }
    
    // Lookup tables for rank conversion (higher index = higher rank)
    const char ranks[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'};
    // since rank_to_index is const, must use .at() to access elements instead of []
    static const std::unordered_map<char, int> rank_to_index = {
        {'2', 0}, {'3', 1}, {'4', 2}, {'5', 3}, {'6', 4},
        {'7', 5}, {'8', 6}, {'9', 7}, {'T', 8}, {'J', 9},
        {'Q', 10}, {'K', 11}, {'A', 12}
    };
    
    // Combine each player's hand with board cards
    std::vector<Card> p0_cards = hands[0];
    p0_cards.insert(p0_cards.end(), board.begin(), board.end());
    
    std::vector<Card> p1_cards = hands[1];
    p1_cards.insert(p1_cards.end(), board.begin(), board.end());
    
    // Precompute rank counts for each player (key = rank index: 0=2, 1=3, ..., 12=A)
    std::map<int, int> p0_rank_count;
    std::map<int, int> p1_rank_count;
    
    // Precompute per-suit sorted rank indices for straight flush detection
    // Maps suit -> sorted vector of rank indices (0=2, 1=3, ..., 12=A)
    std::map<char, std::vector<int>> p0_suit_ranks;
    std::map<char, std::vector<int>> p1_suit_ranks;
    
    for (const Card& card : p0_cards) {
        int rank_idx = rank_to_index.at(card.rank);
        p0_rank_count[rank_idx]++;
        p0_suit_ranks[card.suit].push_back(rank_idx);
    }
    for (auto& [suit, indices] : p0_suit_ranks) {
        std::sort(indices.begin(), indices.end());
    }
    
    for (const Card& card : p1_cards) {
        int rank_idx = rank_to_index.at(card.rank);
        p1_rank_count[rank_idx]++;
        p1_suit_ranks[card.suit].push_back(rank_idx);
    }
    for (auto& [suit, indices] : p1_suit_ranks) {
        std::sort(indices.begin(), indices.end());
    }
    
    // ================================ CHECK FOR STRAIGHT FLUSH ================================
    // Helper function to check for straight flush using precomputed suit_ranks
    // Returns the highest card of the straight flush, or 'X' if none found
    auto check_straight_flush = [&ranks](const std::map<char, std::vector<int>>& suit_ranks) -> char {
        // go through each suit
        for (const auto& [suit, rank_indices] : suit_ranks) {
            if (rank_indices.size() >= 5) {
                // Iterate backwards - first straight found is highest
                int consecutive = 1;
                bool has_ace = rank_indices.back() == 12;
                for (int i = rank_indices.size() - 2; i >= 0; i--) {
                    if (rank_indices[i] == rank_indices[i+1] - 1) {
                        consecutive++;
                        if (consecutive >= 5) {
                            return ranks[rank_indices[i + 4]];  // high card of straight
                        }
                        // Check for wheel (A-2-3-4-5): 4 consecutive ending at index 0, plus Ace
                        // do NOT move this condition outside of the if statement (ugly code)
                        if (consecutive == 4 && rank_indices[i] == 0 && has_ace) {
                            return '5';  // 5-high wheel
                        }
                    } else if (rank_indices[i] != rank_indices[i+1]) {
                        consecutive = 1;
                    }
                }
            }
        }
        return 'X';  // No straight flush
    };
    
    char p0_sf = check_straight_flush(p0_suit_ranks);
    char p1_sf = check_straight_flush(p1_suit_ranks);
    
    // Compare straight flush results
    if (p0_sf != 'X' && p1_sf != 'X') {
        // Both have straight flush - compare ranks by creating cards with dummy suits and comparing them
        Card c0('C', p0_sf);
        Card c1('C', p1_sf);
        if (c0 == c1) { return -1; }
        if (c0 > c1) { return 0; }
        return 1;
    }
    if (p0_sf != 'X') { return 0; }  // Only player 0 has straight flush
    if (p1_sf != 'X') { return 1; }  // Only player 1 has straight flush

    // ================================ CHECK FOR QUADS ================================
    // Helper function to check for quads
    // Returns the rank index of the quads, or -1 if none found
    auto check_quads = [](const std::map<int, int>& rank_count) -> int {
        for (const auto& pair : rank_count) {
            if (pair.second >= 4) { return pair.first; }
        }
        return -1;
    };
    
    int p0_quads = check_quads(p0_rank_count);
    int p1_quads = check_quads(p1_rank_count);
    
    // Compare four of a kind results (higher index = higher rank)
    if (p0_quads != -1 && p1_quads != -1) {
        if (p0_quads > p1_quads) { return 0; }  // higher index = higher rank
        if (p1_quads > p0_quads) { return 1; }
        // Same quads — compare kicker (highest card that isn't the quad rank)
        // only have to check for existence of rank for kicker, since only 1 other card allowed per player
        Card p0_kicker('C', '2');  // dummy init
        Card p1_kicker('C', '2');
        for (const Card& c : p0_cards) {
            if (rank_to_index.at(c.rank) != p0_quads && c > p0_kicker) {
                p0_kicker = c;
            }
        }
        for (const Card& c : p1_cards) {
            if (rank_to_index.at(c.rank) != p1_quads && c > p1_kicker) {
                p1_kicker = c;
            }
        }
        if (p0_kicker > p1_kicker) { return 0; }
        if (p1_kicker > p0_kicker) { return 1; }
        return -1;  // Same quads and same kicker — chop
    }
    if (p0_quads != -1) { return 0; }
    if (p1_quads != -1) { return 1; }
    
    // ================================ CHECK FOR FULL HOUSE ================================
    // Helper lambda to check for full house
    // Returns a tuple of (triple_rank_idx, pair_rank_idx) if full house, or (-1, -1) if no full house
    // Iterate backwards (rbegin/rend) so first found = highest
    auto check_full_house = [](const std::map<int, int>& rank_count) -> std::tuple<int, int> {
        int triple_rank = -1;
        int pair_rank = -1;
        
        // Find the highest triple (first one found when iterating backwards)
        for (auto it = rank_count.rbegin(); it != rank_count.rend(); ++it) {
            if (it->second >= 3) {
                triple_rank = it->first;
                break;
            }
        }
        
        if (triple_rank == -1) { return {-1, -1}; }  // No triple found
        
        // Find the highest pair (different rank than triple)
        for (auto it = rank_count.rbegin(); it != rank_count.rend(); ++it) {
            if (it->first != triple_rank && it->second >= 2) {
                pair_rank = it->first;
                break;
            }
        }
        
        if (pair_rank == -1) { return {-1, -1}; }  // No pair found
        
        return {triple_rank, pair_rank};
    };
    
    auto [p0_fh_triple, p0_fh_pair] = check_full_house(p0_rank_count);
    auto [p1_fh_triple, p1_fh_pair] = check_full_house(p1_rank_count);
    
    // Compare full house results (higher index = higher rank)
    if (p0_fh_triple != -1 && p1_fh_triple != -1) {
        if (p0_fh_triple > p1_fh_triple) { return 0; }    // Player 0 wins
        if (p1_fh_triple > p0_fh_triple) { return 1; }    // Player 1 wins
        // Triple ranks are equal, compare pair ranks
        if (p0_fh_pair > p1_fh_pair) { return 0; }
        if (p1_fh_pair > p0_fh_pair) { return 1; }
        return -1;  // Tie
    }
    if (p0_fh_triple != -1) { return 0; }  // Only player 0 has full house
    if (p1_fh_triple != -1) { return 1; }  // Only player 1 has full house
    
    // ================================ CHECK FOR FLUSH ================================
    // Helper function to check for flush using precomputed suit_ranks
    // Returns the highest rank index if flush found, -1 if no flush
    auto check_flush = [](const std::map<char, std::vector<int>>& suit_ranks) -> int {
        for (const auto& [suit, rank_indices] : suit_ranks) {
            if (rank_indices.size() >= 5) {
                return rank_indices.back();  // Highest rank (already sorted ascending)
            }
        }
        return -1;  // No flush
    };
    
    int p0_flush = check_flush(p0_suit_ranks);
    int p1_flush = check_flush(p1_suit_ranks);
    
    // Compare flush results (higher index = higher rank)
    if (p0_flush != -1 && p1_flush != -1) {
        if (p0_flush > p1_flush) { return 0; }
        if (p1_flush > p0_flush) { return 1; }
        return -1;  // Tie
    }
    if (p0_flush != -1) { return 0; }  // Only player 0 has flush
    if (p1_flush != -1) { return 1; }  // Only player 1 has flush
    
    // ================================ CHECK FOR STRAIGHT ================================
    // Helper function to check for straight
    // Returns the highest rank index of the straight, or -1 if no straight
    auto check_straight = [](const std::map<int, int>& rank_count) -> int {
        // Get sorted rank indices that exist
        std::vector<int> rank_indices;
        for (const auto& [rank_idx, count] : rank_count) {
            rank_indices.push_back(rank_idx);
        }
        // Already sorted since std::map is ordered
        
        if (rank_indices.size() < 5) { return -1; }
        
        // Iterate backwards to find highest straight first
        int consecutive = 1;
        bool has_ace = rank_indices.back() == 12;
        for (int i = rank_indices.size() - 2; i >= 0; i--) {
            if (rank_indices[i] == rank_indices[i+1] - 1) {
                consecutive++;
                if (consecutive >= 5) {
                    return rank_indices[i + 4];  // High card of straight
                }
                // Check for wheel (A-2-3-4-5): 4 consecutive ending at index 0, plus Ace
                if (consecutive == 4 && rank_indices[i] == 0 && has_ace) {
                    return 3;  // 5-high wheel (index 3 = '5')
                }
            } else if (rank_indices[i] != rank_indices[i+1]) {
                consecutive = 1;
            }
        }
        return -1;  // No straight
    };
    
    int p0_straight = check_straight(p0_rank_count);
    int p1_straight = check_straight(p1_rank_count);
    
    // Compare straight results (higher index = higher rank)
    if (p0_straight != -1 && p1_straight != -1) {
        if (p0_straight > p1_straight) { return 0; }
        if (p1_straight > p0_straight) { return 1; }
        return -1;  // Tie
    }
    if (p0_straight != -1) { return 0; }  // Only player 0 has straight
    if (p1_straight != -1) { return 1; }  // Only player 1 has straight
    
    // ================================ CHECK FOR THREE OF A KIND ================================
    // Helper function to check for three of a kind
    // Returns the highest rank index of the triple, or -1 if none found
    auto check_trips = [](const std::map<int, int>& rank_count) -> int {
        int trips_rank = -1;
        for (const auto& [rank_idx, count] : rank_count) {
            if (count >= 3) {
                trips_rank = rank_idx;
            }
        }
        return trips_rank;
    };
    
    int p0_trips = check_trips(p0_rank_count);
    int p1_trips = check_trips(p1_rank_count);
    
    // Compare three of a kind results (higher index = higher rank)
    if (p0_trips != -1 && p1_trips != -1) {
        if (p0_trips > p1_trips) { return 0; }
        if (p1_trips > p0_trips) { return 1; }
        // Same trips rank — compare top 2 kickers (highest non-trips ranks)
        // only have to check for existence of rank for kicker, since if another rank was paired,
        // the player would have a full house instead of three of a kind
        std::vector<int> p0_kickers;
        for (auto it = p0_rank_count.rbegin(); it != p0_rank_count.rend(); ++it) {
            if (it->first != p0_trips) {
                p0_kickers.push_back(it->first);
                if (p0_kickers.size() == 2) { break; }
            }
        }
        std::vector<int> p1_kickers;
        for (auto it = p1_rank_count.rbegin(); it != p1_rank_count.rend(); ++it) {
            if (it->first != p1_trips) {
                p1_kickers.push_back(it->first);
                if (p1_kickers.size() == 2) { break; }
            }
        }
        // Compare kickers in order (highest first)
        for (size_t i = 0; i < 2 && i < p0_kickers.size() && i < p1_kickers.size(); ++i) {
            if (p0_kickers[i] > p1_kickers[i]) { return 0; }
            if (p1_kickers[i] > p0_kickers[i]) { return 1; }
        }
        return -1;  // Tie — same trips and same kickers
    }
    if (p0_trips != -1) { return 0; }  // Only player 0 has three of a kind
    if (p1_trips != -1) { return 1; }  // Only player 1 has three of a kind
    
    // ================================ CHECK FOR TWO PAIR ================================
    // Helper function to check for two pair
    // Returns tuple of (highest pair rank, second pair rank), or (-1, -1) if no two pair
    auto check_two_pair = [](const std::map<int, int>& rank_count) -> std::tuple<int, int> {
        int high_pair = -1;
        int low_pair = -1;
        
        // Iterate forward, last two pairs found will be the highest two
        for (const auto& [rank_idx, count] : rank_count) {
            if (count >= 2) {
                low_pair = high_pair;
                high_pair = rank_idx;
            }
        }
        
        if (low_pair == -1) { return {-1, -1}; }  // Less than two pairs
        return {high_pair, low_pair};
    };
    
    auto [p0_pair1, p0_pair2] = check_two_pair(p0_rank_count);
    auto [p1_pair1, p1_pair2] = check_two_pair(p1_rank_count);
    
    // Compare two pair results (higher index = higher rank)
    if (p0_pair1 != -1 && p1_pair1 != -1) {
        if (p0_pair1 > p1_pair1) { return 0; }
        if (p1_pair1 > p0_pair1) { return 1; }
        // High pairs equal, compare second pair
        if (p0_pair2 > p1_pair2) { return 0; }
        if (p1_pair2 > p0_pair2) { return 1; }
        // Both pairs equal — compare kicker (highest card not part of either pair)
        // only have to check for existence of rank for kicker, since only 1 other card allowed per player
        int p0_kicker = -1;
        for (auto it = p0_rank_count.rbegin(); it != p0_rank_count.rend(); ++it) {
            if (it->first != p0_pair1 && it->first != p0_pair2) {
                p0_kicker = it->first;
                break;
            }
        }
        int p1_kicker = -1;
        for (auto it = p1_rank_count.rbegin(); it != p1_rank_count.rend(); ++it) {
            if (it->first != p1_pair1 && it->first != p1_pair2) {
                p1_kicker = it->first;
                break;
            }
        }
        assert(p0_kicker != -1 && "Two pair kicker not found for player 0");
        assert(p1_kicker != -1 && "Two pair kicker not found for player 1");
        if (p0_kicker > p1_kicker) { return 0; }
        if (p1_kicker > p0_kicker) { return 1; }
        return -1;  // Tie — same two pair and same kicker
    }
    if (p0_pair1 != -1) { return 0; }  // Only player 0 has two pair
    if (p1_pair1 != -1) { return 1; }  // Only player 1 has two pair
    
    // ================================ CHECK FOR PAIR ================================
    // Helper function to check for pair
    // Returns the highest rank index of the pair, or -1 if none found
    auto check_pair = [](const std::map<int, int>& rank_count) -> int {
        int pair_rank = -1;
        for (const auto& [rank_idx, count] : rank_count) {
            if (count >= 2) {
                pair_rank = rank_idx;
            }
        }
        return pair_rank;
    };
    
    int p0_pair = check_pair(p0_rank_count);
    int p1_pair = check_pair(p1_rank_count);
    
    // Compare pair results (higher index = higher rank)
    if (p0_pair != -1 && p1_pair != -1) {
        if (p0_pair > p1_pair) { return 0; }
        if (p1_pair > p0_pair) { return 1; }
        // Same pair — compare top 3 kickers (highest non-pair ranks)
        // only have to check for existence of another rank for kicker. since if another rank was paired,
        // the player would have two-pair instead of a pair
        std::vector<int> p0_kickers;
        for (auto it = p0_rank_count.rbegin(); it != p0_rank_count.rend(); ++it) {
            if (it->first != p0_pair) {
                p0_kickers.push_back(it->first);
                if (p0_kickers.size() == 3) { break; }
            }
        }
        std::vector<int> p1_kickers;
        for (auto it = p1_rank_count.rbegin(); it != p1_rank_count.rend(); ++it) {
            if (it->first != p1_pair) {
                p1_kickers.push_back(it->first);
                if (p1_kickers.size() == 3) { break; }
            }
        }
        // Compare kickers in order (highest first)
        // i < kickers.size() is defensive and we should never activate it
        for (size_t i = 0; i < 3 && i < p0_kickers.size() && i < p1_kickers.size(); ++i) {
            if (p0_kickers[i] > p1_kickers[i]) { return 0; }
            if (p1_kickers[i] > p0_kickers[i]) { return 1; }
        }
        return -1;  // Tie — same pair and same kickers
    }
    if (p0_pair != -1) { return 0; }  // Only player 0 has pair
    if (p1_pair != -1) { return 1; }  // Only player 1 has pair
    
    // ================================ CHECK FOR HIGH CARD ================================
    // Compare top 5 cards by rank (highest first)
    // only have to check for existence of rank for kicker, since if any of the cards were paired,
    // they would have a pair on not high card
    std::vector<int> p0_highs;
    for (auto it = p0_rank_count.rbegin(); it != p0_rank_count.rend(); ++it) {
        p0_highs.push_back(it->first);
        if (p0_highs.size() == 5) { break; }
    }
    std::vector<int> p1_highs;
    for (auto it = p1_rank_count.rbegin(); it != p1_rank_count.rend(); ++it) {
        p1_highs.push_back(it->first);
        if (p1_highs.size() == 5) { break; }
    }
    for (size_t i = 0; i < 5; ++i) {
        if (p0_highs[i] > p1_highs[i]) { return 0; }
        if (p1_highs[i] > p0_highs[i]) { return 1; }
    }
    return -1;  // Tie — same top 5 cards
}

void PokerKit::deal_board(Card card) {
    if (game_over) {
        throw std::runtime_error("Game is over");
    }
    if (board.size() >= 5) {
        throw std::runtime_error("Board already has 5 cards");
    }
    
    // Check card exists in deck and remove it
    auto it = std::find_if(deck.begin(), deck.end(), [&card](const Card& deck_card) {
        return deck_card.suit == card.suit && deck_card.rank == card.rank;
    });
    if (it == deck.end()) {
        throw std::runtime_error("Card not found in deck");
    }
    deck.erase(it);
    
    board.push_back(card);
    
    // Start new betting round after flop (3), turn (4), or river (5)
    if (board.size() == 3 || board.size() == 4 || board.size() == 5) {
        bets.push_back({});
    }
    
    // If river is dealt and betting is already locked (all-in was called), end the game
    if (board.size() == 5 && !can_still_bet) {
        game_over = true;
        settle_pot();
    }
}

void PokerKit::deal_board() {
    if (deck.empty()) {
        throw std::runtime_error("No cards left in deck");
    }
    Card card = deck.back();
    deal_board(card);
}

void PokerKit::settle_pot() {
    // Check if game is actually done (fold or all 4 streets completed)
    bool has_fold = !bets.back().empty() && bets.back().back().type == 'f';
    bool all_streets_done = bets.size() == 4;
    
    if (!(has_fold || all_streets_done)) {
        throw std::runtime_error("Game is not over yet");
    }

    assert (game_over && "Game is not over");
    
    int8_t winner = calculate_winner();
    
    if (winner == 0) {
        stacks[0] += pot;
    } else if (winner == 1) {
        stacks[1] += pot;
    } else {
        // Tie - split pot
        stacks[0] += pot / 2;
        stacks[1] += pot / 2;
    }
    
    pot = 0;
}

std::string PokerKit::debug_print_history() const {
    assert(initial_stacks[0] == initial_stacks[1] && "Initial stacks must be equal");

    std::string history_str = "Betting history: ";
    uint16_t running_pot = 0;
    for (size_t street = 0; street < bets.size(); street++) {
        history_str += "[Street " + std::to_string(street) + ": ";
        for (const Action& a : bets[street]) {
            history_str += std::string(a) + " ";
        }
        // Calculate pot after this street from the last 2 actions
        const auto& street_actions = bets[street];
        if (street_actions.size() >= 2) {
            const Action& last = street_actions.back();
            const Action& second_last = street_actions[street_actions.size() - 2];
            if (last.type == 'c') {
                running_pot += 2 * second_last.amount;
            }
        }
        uint16_t chips_each = initial_stacks[0] - running_pot / 2;
        history_str += "(pot: " + std::to_string(running_pot) + ", chips: " + std::to_string(chips_each) + ")] ";
    }
    return history_str;
}
