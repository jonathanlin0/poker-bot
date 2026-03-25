#ifndef SERVER_GAME_UTIL_HPP
#define SERVER_GAME_UTIL_HPP

#include <array>
#include <string>
#include <vector>
#include "card.hpp"
#include "action.hpp"

/*
 * In-memory representation of a single row from the `hands` SQLite table.
 * Loaded from DB, mutated as actions happen, then written back.
*/
struct HandData {
    int hand_id;
    int player_id;
    int player_seat;
    std::array<Card, 2> player_cards;
    std::array<Card, 2> bot_cards;
    std::vector<Card> deck;
    std::vector<Card> board;
    std::vector<std::vector<Action>> history;
    bool is_complete;
    std::string bot_version;
};

std::string read_file(const std::string& path);

// Returns true if the username contains only alphanumeric characters and is non-empty.
bool is_valid_username(const std::string& username);

std::string card_to_str(const Card& c);
std::array<std::array<Card, 2>, 2> get_cards(const HandData& hand);

template<typename Container>
std::string serialize_cards(const Container& cards);
std::string serialize_history(const std::vector<std::vector<Action>>& history);
std::vector<Card> deserialize_cards(const std::string& json);
std::vector<std::vector<Action>> deserialize_history(const std::string& json);

#endif // SERVER_GAME_UTIL_HPP
