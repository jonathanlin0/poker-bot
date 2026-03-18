#include "../include/infoset_calculator.hpp"
#include <array>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <sstream>

using std::array;
using std::getline;
using std::invalid_argument;
using std::istringstream;
using std::max;
using std::max_element;
using std::stoi;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

namespace {

const string BUCKET_DELIMITER = " ";

char value_to_rank(uint8_t value) {
    switch (value) {
        case 0:  return '2';
        case 1:  return '3';
        case 2:  return '4';
        case 3:  return '5';
        case 4:  return '6';
        case 5:  return '7';
        case 6:  return '8';
        case 7:  return '9';
        case 8:  return 'T';
        case 9:  return 'J';
        case 10: return 'Q';
        case 11: return 'K';
        case 12: return 'A';
        default: throw invalid_argument("Invalid rank value: " + to_string(value));
    }
}

uint8_t rank_to_value(char rank) {
    switch (rank) {
        case '2': return 0;
        case '3': return 1;
        case '4': return 2;
        case '5': return 3;
        case '6': return 4;
        case '7': return 5;
        case '8': return 6;
        case '9': return 7;
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
        default: throw invalid_argument("Invalid rank: " + string{rank});
    }
}

uint8_t suit_to_index(char suit) {
    switch (suit) {
        case 'H': return 0;
        case 'S': return 1;
        case 'D': return 2;
        case 'C': return 3;
        default: throw invalid_argument("Invalid suit: " + string{suit});
    }
}

// note: this maps the output of rank_to_value, not the rank itself
const unordered_map<uint8_t, uint8_t> RANK_TO_GROUP = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0},     // 2345 -> group 0
    {4, 1}, {5, 1}, {6, 1}, {7, 1},     // 6789 -> group 1
    {8, 2}, {9, 2}, {10, 2},               // TJQ   -> group 2
    {11, 3}, {12, 3}                          // KA  -> group 3
};

const unordered_map<uint8_t, string> GROUP_TO_RANKS = {
    {0, "2, 3, 4, or 5"},
    {1, "6, 7, 8, or 9"},
    {2, "T, J, or Q"},
    {3, "K or A"}
};

const unordered_map<string, uint8_t> HAND_RANKS = {
    {"high card", 0},
    {"pair", 1},
    {"two pair", 2},
    {"trips", 3},
    {"straight", 4},
    {"flush", 5},
    {"full house", 6},
    {"quads", 7},
    {"straight flush", 8}
};

const unordered_map<uint8_t, string> HAND_RANK_NAMES = {
    {0, "high card"},
    {1, "pair"},
    {2, "two pair"},
    {3, "trips"},
    {4, "straight"},
    {5, "flush"},
    {6, "full house"},
    {7, "quads"},
    {8, "straight flush"}
};

struct BoardTexture {
    bool is_board_paired;
    uint8_t max_rank;                    // rank group of highest card (0-3, see RANK_TO_GROUP)
    uint8_t max_suit_count;              // max number of cards of the same suit
    uint8_t max_straight_window;         // max distinct board ranks in any 5-consecutive-rank window (incl. wheel)

    BoardTexture() = default;

    BoardTexture(const string& s) {
        istringstream ss(s); // convert string to a stream to parse
        string token;

        getline(ss, token, '-');
        is_board_paired = static_cast<bool>(stoi(token));

        getline(ss, token, '-');
        max_rank = static_cast<uint8_t>(stoi(token));

        getline(ss, token, '-');
        max_suit_count = static_cast<uint8_t>(stoi(token));

        getline(ss, token, '-');
        max_straight_window = static_cast<uint8_t>(stoi(token));
    }

    operator string() const {
        return to_string(is_board_paired) + "-"
             + to_string(max_rank) + "-"
             + to_string(max_suit_count) + "-"
             + to_string(max_straight_window);
    }

    string pretty_print() const {
        string result = "Board is " + string(is_board_paired ? "paired" : "not paired");
        result += ", max rank group is " + GROUP_TO_RANKS.at(max_rank);
        result += ", " + to_string(max_suit_count) + " cards of the same suit";
        result += ", " + to_string(max_straight_window) + "/5 cards in best straight window";
        return result;
    }
};

struct PreflopHandState {
    uint8_t higher_rank_group;  // see RANK_TO_GROUP / GROUP_TO_RANKS
    uint8_t lower_rank_group;
    bool is_suited;

    PreflopHandState(uint8_t higher, uint8_t lower, bool suited)
        : higher_rank_group(higher), lower_rank_group(lower), is_suited(suited) {}

    PreflopHandState(const string& s) {
        istringstream ss(s);
        string token;

        getline(ss, token, '-');
        higher_rank_group = static_cast<uint8_t>(stoi(token));

        getline(ss, token, '-');
        lower_rank_group = static_cast<uint8_t>(stoi(token));

        getline(ss, token, '-');
        is_suited = static_cast<bool>(stoi(token));
    }

    operator string() const {
        return to_string(higher_rank_group) + "-"
             + to_string(lower_rank_group) + "-"
             + to_string(is_suited);
    }

    string pretty_print() const {
        string result = "Higher card: " + GROUP_TO_RANKS.at(higher_rank_group);
        result += ", lower card: " + GROUP_TO_RANKS.at(lower_rank_group);
        result += ", " + string(is_suited ? "suited" : "offsuit");
        return result;
    }
};

struct PostflopHandState {
    uint8_t made_hand;  // see HAND_RANKS for mapping
    uint8_t kicker;     // rank group of highest card not in the made hand (0-3, see RANK_TO_GROUP)
                        // (unused for straight, flush, full house, quads, straight flush)

    PostflopHandState(uint8_t made_hand, uint8_t kicker) : made_hand(made_hand), kicker(kicker) {}

    PostflopHandState(const string& s) {
        istringstream ss(s);
        string token;

        getline(ss, token, '-');
        made_hand = static_cast<uint8_t>(stoi(token));

        getline(ss, token, '-');
        kicker = static_cast<uint8_t>(stoi(token));
    }

    operator string() const {
        if (made_hand >= 4) {
            return to_string(made_hand);
        }
        return to_string(made_hand) + "-" + to_string(kicker);
    }

    string pretty_print() const {
        string result = "Made hand: " + HAND_RANK_NAMES.at(made_hand);
        if (made_hand < 4) {
            result += ", kicker group: " + GROUP_TO_RANKS.at(kicker);
        } else {
            result += ", no kicker";
        }
        return result;
    }
};

struct BettingAggression {
    array<vector<uint8_t>, 2> raise_counts;  // [player][street] = number of raises

    BettingAggression() = default;

    BettingAggression(const string& s) {
        istringstream ss(s);
        string token;
        // Each token is 2 digits: first digit = p0 raises, second digit = p1 raises
        while (getline(ss, token, '-')) {
            raise_counts[0].push_back(static_cast<uint8_t>(token[0] - '0'));
            raise_counts[1].push_back(static_cast<uint8_t>(token[1] - '0'));
        }
    }

    operator string() const {
        string result;
        for (size_t i = 0; i < raise_counts[0].size(); i++) {
            if (!result.empty()) result += "-";
            result += to_string(raise_counts[0][i]);
            result += to_string(raise_counts[1][i]);
        }
        return result;
    }

    string pretty_print() const {
        const array<string, 4> street_names = {"preflop", "flop", "turn", "river"};
        string result;
        for (int p = 0; p < 2; p++) {
            string player_name = (p == 0) ? "SB" : "BB";
            for (size_t s = 0; s < raise_counts[p].size(); s++) {
                if (!result.empty()) result += ", ";
                result += player_name + " raised " + to_string(raise_counts[p][s])
                        + " time" + (raise_counts[p][s] != 1 ? "s" : "")
                        + " on " + street_names[s];
            }
        }
        return result;
    }
};

BoardTexture get_board_texture(const PokerKit& game) {
    const auto& board = game.get_board();
    assert(!board.empty());

    BoardTexture texture{};
    texture.is_board_paired = false;
    texture.max_rank = 0;
    texture.max_suit_count = 0;
    texture.max_straight_window = 0;

    // Build rank counts
    // maps rank to count
    array<uint8_t, 13> rank_counts{};  // rank_value -> count
    for (const auto& card : board) {
        rank_counts[rank_to_value(card.rank)]++;
    }

    // Build suit counts
    // maps suit to count
    array<uint8_t, 4> suit_counts{};  // suit_index -> count
    for (const auto& card : board) {
        suit_counts[suit_to_index(card.suit)]++;
    }

    // Max rank on the board (bucketed to group)
    for (int i = 12; i >= 0; i--) {
        if (rank_counts[i] > 0) {
            texture.max_rank = RANK_TO_GROUP.at(static_cast<uint8_t>(i));
            break;
        }
    }

    // Max cards of the same suit
    texture.max_suit_count = *max_element(suit_counts.begin(), suit_counts.end());

    // Check if board is paired
    for (auto count : rank_counts) {
        if (count >= 2) {
            texture.is_board_paired = true;
            break;
        }
    }

    // Sliding window for straight viability: windows [i, i+4] for i in [0, 8]
    // Counts distinct ranks present in each window
    uint8_t max_window = 0;
    for (int i = 0; i <= 8; i++) {
        uint8_t count = 0;
        for (int j = i; j <= i + 4; j++) {
            if (rank_counts[j] > 0) count++;
        }
        max_window = max(max_window, count);
    }

    // Separate wheel check: A-2-3-4-5 = rank_values {12, 0, 1, 2, 3}
    uint8_t wheel_count = 0;
    if (rank_counts[12] > 0) wheel_count++;  // A
    if (rank_counts[0] > 0)  wheel_count++;  // 2
    if (rank_counts[1] > 0)  wheel_count++;  // 3
    if (rank_counts[2] > 0)  wheel_count++;  // 4
    if (rank_counts[3] > 0)  wheel_count++;  // 5

    texture.max_straight_window = max(max_window, wheel_count);

    return texture;
}

PreflopHandState get_preflop_hand_state(const PokerKit& game, uint8_t player) {
    const auto& hand = game.get_hand(player);

    uint8_t rv0 = rank_to_value(hand[0].rank);
    uint8_t rv1 = rank_to_value(hand[1].rank);
    uint8_t group0 = RANK_TO_GROUP.at(rv0);
    uint8_t group1 = RANK_TO_GROUP.at(rv1);

    uint8_t higher = (group0 >= group1) ? group0 : group1;
    uint8_t lower  = (group0 >= group1) ? group1 : group0;
    bool suited = (hand[0].suit == hand[1].suit);

    return PreflopHandState(higher, lower, suited);
}

PostflopHandState get_postflop_hand_state(const PokerKit& game, uint8_t player) {
    const auto& hand = game.get_hand(player);
    const auto& board = game.get_board();

    // Build rank counts and per-suit ranks from all cards (hand + board)
    array<uint8_t, 13> rank_counts{};
    array<uint8_t, 4> suit_counts{};
    array<vector<uint8_t>, 4> suit_ranks{};  // ranks grouped by suit

    for (const auto& card : hand) {
        uint8_t rv = rank_to_value(card.rank);
        uint8_t si = suit_to_index(card.suit);
        rank_counts[rv]++;
        suit_counts[si]++;
        suit_ranks[si].push_back(rv);
    }
    for (const auto& card : board) {
        uint8_t rv = rank_to_value(card.rank);
        uint8_t si = suit_to_index(card.suit);
        rank_counts[rv]++;
        suit_counts[si]++;
        suit_ranks[si].push_back(rv);
    }

    // Check for straight: returns highest card rank_value, or -1 if no straight
    auto check_straight = [](const array<uint8_t, 13>& counts) -> int8_t {
        // Normal straights: windows [i, i+4], check from top down
        for (int i = 8; i >= 0; i--) {
            bool all_present = true;
            for (int j = i; j <= i + 4; j++) {
                if (counts[j] == 0) { all_present = false; break; }
            }
            if (all_present) return static_cast<int8_t>(i + 4);
        }
        // Wheel: A-2-3-4-5
        if (counts[12] > 0 && counts[0] > 0 && counts[1] > 0
            && counts[2] > 0 && counts[3] > 0) {
            return rank_to_value('5');  // 5 is the high card of the wheel (rank_value 3)
        }
        return -1;
    };

    // Find flush suit (if any has 5+ cards)
    int8_t flush_suit = -1;
    for (int i = 0; i < 4; i++) {
        if (suit_counts[i] >= 5) {
            flush_suit = static_cast<int8_t>(i);
            break; // only 1 flush can exist at a time
        }
    }

    // Find kicker: highest rank not in a set of excluded ranks
    auto find_kicker = [&](const vector<uint8_t>& exclude_ranks) -> uint8_t {
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 0) continue;
            bool is_curr_rank_excluded = false;
            for (uint8_t r : exclude_ranks) {
                if (r == static_cast<uint8_t>(i)) { is_curr_rank_excluded = true; break; }
            }
            if (!is_curr_rank_excluded) return RANK_TO_GROUP.at(static_cast<uint8_t>(i));
        }
        return 0;
    };

    // Check for straight flush
    if (flush_suit >= 0) {
        array<uint8_t, 13> flush_rank_counts{};
        for (uint8_t rv : suit_ranks[flush_suit]) {
            flush_rank_counts[rv]++;
        }
        if (check_straight(flush_rank_counts) >= 0) {
            return {HAND_RANKS.at("straight flush"), 0};
        }
    }

    // Categorize ranks by their count (sorted rank high to low)
    vector<uint8_t> quad_ranks, trip_ranks, pair_ranks;
    for (int i = 12; i >= 0; i--) {
        if (rank_counts[i] == 4) quad_ranks.push_back(static_cast<uint8_t>(i));
        else if (rank_counts[i] == 3) trip_ranks.push_back(static_cast<uint8_t>(i));
        else if (rank_counts[i] == 2) pair_ranks.push_back(static_cast<uint8_t>(i));
    }

    // Check for quads
    if (!quad_ranks.empty()) {
        return {HAND_RANKS.at("quads"), 0};
    }

    // Check for full house
    if (!trip_ranks.empty() && (!pair_ranks.empty() || trip_ranks.size() >= 2)) {
        return {HAND_RANKS.at("full house"), 0};
    }

    // Check for flush
    if (flush_suit >= 0) {
        return {HAND_RANKS.at("flush"), 0};
    }

    // Check for straight
    if (check_straight(rank_counts) >= 0) {
        return {HAND_RANKS.at("straight"), 0};
    }

    // Check for three of a kind
    if (!trip_ranks.empty()) {
        return {HAND_RANKS.at("trips"), find_kicker({trip_ranks[0]})};
    }

    // Check for two pair
    if (pair_ranks.size() >= 2) {
        return {HAND_RANKS.at("two pair"), find_kicker({pair_ranks[0], pair_ranks[1]})};
    }

    // Check for pair
    if (pair_ranks.size() == 1) {
        return {HAND_RANKS.at("pair"), find_kicker({pair_ranks[0]})};
    }

    // High card
    return {HAND_RANKS.at("high card"), find_kicker({})};
}

BettingAggression get_betting_aggression(const PokerKit& game) {
    const auto& bets = game.get_bets();

    BettingAggression aggression{};
    aggression.raise_counts[0] = {};
    aggression.raise_counts[1] = {};

    for (size_t street = 0; street < bets.size(); street++) {
        uint8_t p0_raises = 0;
        uint8_t p1_raises = 0;

        for (size_t i = 0; i < bets[street].size(); i++) {
            if (bets[street][i].type != 'r') continue;

            // Even index = SB (p0), odd index = BB (p1)
            if (i % 2 == 0){
                p0_raises++;
            } else {
                p1_raises++;
            }
        }

        aggression.raise_counts[0].push_back(p0_raises);
        aggression.raise_counts[1].push_back(p1_raises);
    }

    return aggression;
}

string get_actions_as_string(const vector<Action>& possible_actions) {
    string result;
    for (const Action& a : possible_actions) {
        string action_str = static_cast<string>(a);

        // remove everything after the semicolon for raises
        // if (a.type == 'r') {
        //     size_t semi = action_str.find(';');
        //     if (semi != string::npos) {
        //         action_str = action_str.substr(0, semi);
        //     }
        // }
        result += action_str;
    }
    return result;
}

} // anonymous namespace so only this file can this stuff

string InfosetCalculator::get_infoset_from_game(const PokerKit& game, uint8_t player, const vector<Action>& possible_actions) {

    // <player> <betting round> <board texture> <hand state> <betting aggression> <possible actions>
    BettingAggression betting_aggression = get_betting_aggression(game);

    string hand_and_board_state_str;
    if (game.get_betting_street() == 0) {
        PreflopHandState hand_state = get_preflop_hand_state(game, player);
        hand_and_board_state_str = static_cast<string>(hand_state);
    } else {
        BoardTexture board_texture = get_board_texture(game);
        PostflopHandState hand_state = get_postflop_hand_state(game, player);
        hand_and_board_state_str = static_cast<string>(board_texture) + BUCKET_DELIMITER
                       + static_cast<string>(hand_state);
    }

    return to_string(player) + BUCKET_DELIMITER
         + to_string(game.get_betting_street()) + BUCKET_DELIMITER // not necessary since the nodes are separated by street alr, but just for clarity when printing the infoset in local files and for debugging
         + hand_and_board_state_str + BUCKET_DELIMITER
         + static_cast<string>(betting_aggression) + BUCKET_DELIMITER
         + get_actions_as_string(possible_actions); // to ensure that there's no illegal bucket collision
}
