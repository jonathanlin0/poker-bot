#ifndef SERVER_DB_UTIL_HPP
#define SERVER_DB_UTIL_HPP

#include <sqlite3.h>
#include <optional>
#include <string>
#include <vector>
#include "server_game_util.hpp"

extern sqlite3* db;

// Database operations
void init_db();
// Inserts a new player or finds an existing one. Returns the player_id.
int upsert_player(const std::string& username);
int find_player_id(const std::string& username);
int get_hand_count(int player_id);
std::optional<HandData> get_active_hand(int player_id);
HandData create_hand(int player_id);
void update_hand_db(const HandData& hand);
std::vector<HandData> get_past_hands(int player_id, int limit = 20);
// TODO: currently an expensive-ish operation due to calling build_game() n times
float get_total_profit(int player_id);

struct PlayerStats {
    std::string username;
    int hands;
    float profit;
};
// Returns all player stats
std::vector<PlayerStats> get_all_player_stats();

#endif // SERVER_DB_UTIL_HPP
