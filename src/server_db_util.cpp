#include "../include/server_db_util.hpp"
#include "../include/config.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/util.hpp"
#include <stdexcept>
#include <string>

using std::array;
using std::optional;
using std::runtime_error;
using std::string;
using std::vector;

sqlite3* db = nullptr;


void init_db() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            created_at TEXT DEFAULT (datetime('now'))
        );
        CREATE TABLE IF NOT EXISTS hands (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id INTEGER NOT NULL,
            player_seat INTEGER NOT NULL,
            player_cards TEXT NOT NULL,
            bot_cards TEXT NOT NULL,
            deck TEXT NOT NULL,
            board TEXT NOT NULL DEFAULT '[]',
            history TEXT NOT NULL DEFAULT '[[]]',
            is_complete INTEGER NOT NULL DEFAULT 0,
            bot_version TEXT NOT NULL,
            created_at TEXT DEFAULT (datetime('now')),
            FOREIGN KEY (player_id) REFERENCES players(id)
        );
    )";
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) {
        string msg = err;
        sqlite3_free(err);
        throw runtime_error("Failed to create tables: " + msg);
    }
}

int upsert_player(const string& username) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM players WHERE username = ?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    int player_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        player_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (player_id != -1) { return player_id; }

    sqlite3_prepare_v2(db, "INSERT INTO players (username) VALUES (?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

int find_player_id(const string& username) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM players WHERE username = ?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int player_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        player_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return player_id;
}

int get_hand_count(int player_id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM hands WHERE player_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, player_id);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

static array<Card, 2> to_card_pair(const string& json) {
    auto v = deserialize_cards(json);
    return {v[0], v[1]};
}

static HandData load_hand_from_row(sqlite3_stmt* stmt, int player_id) {
    return HandData{
        sqlite3_column_int(stmt, 0),
        player_id,
        sqlite3_column_int(stmt, 1),
        to_card_pair(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))),
        to_card_pair(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))),
        deserialize_cards(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))),
        deserialize_cards(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))),
        deserialize_history(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))),
        sqlite3_column_int(stmt, 7) != 0,
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8))
    };
}

optional<HandData> get_active_hand(int player_id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, player_seat, player_cards, bot_cards, deck, board, history, is_complete, bot_version "
        "FROM hands WHERE player_id = ? AND is_complete = 0 LIMIT 1",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, player_id);

    optional<HandData> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = load_hand_from_row(stmt, player_id);
    }
    sqlite3_finalize(stmt);
    return result;
}

HandData create_hand(int player_id) {
    int hand_count = get_hand_count(player_id);
    int player_seat = hand_count % 2;

    vector<Card> deck = get_new_deck(true);
    Card p0 = deck.back(); deck.pop_back();
    Card p1 = deck.back(); deck.pop_back();
    Card b0 = deck.back(); deck.pop_back();
    Card b1 = deck.back(); deck.pop_back();

    array<Card, 2> player_cards = {p0, p1};
    array<Card, 2> bot_cards = {b0, b1};

    string pc_json = serialize_cards(player_cards);
    string bc_json = serialize_cards(bot_cards);
    string deck_json = serialize_cards(deck);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO hands (player_id, player_seat, player_cards, bot_cards, deck, board, history, is_complete, bot_version) "
        "VALUES (?, ?, ?, ?, ?, '[]', '[[]]', 0, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, player_seat);
    sqlite3_bind_text(stmt, 3, pc_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, bc_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, deck_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, BOT_VERSION.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return HandData{
        static_cast<int>(sqlite3_last_insert_rowid(db)), // safe due to mutex locking in server.cpp
        player_id, player_seat, player_cards, bot_cards,
        deck, {}, {{}}, false, BOT_VERSION
    };
}

void update_hand_db(const HandData& hand) {
    string history_json = serialize_history(hand.history);
    string board_json = serialize_cards(hand.board);
    string deck_json = serialize_cards(hand.deck);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "UPDATE hands SET history = ?, board = ?, deck = ?, is_complete = ? WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, history_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, board_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, deck_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, hand.is_complete ? 1 : 0);
    sqlite3_bind_int(stmt, 5, hand.hand_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

vector<HandData> get_past_hands(int player_id, int limit) {
    vector<HandData> hands;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, player_seat, player_cards, bot_cards, deck, board, history, is_complete, bot_version "
        "FROM hands WHERE player_id = ? AND is_complete = 1 "
        "ORDER BY created_at DESC LIMIT ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hands.push_back(load_hand_from_row(stmt, player_id));
    }
    sqlite3_finalize(stmt);
    return hands;
}

float get_total_profit(int player_id) {
    float total = 0.0f;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT player_seat, player_cards, bot_cards, deck, board, history "
        "FROM hands WHERE player_id = ? AND is_complete = 1",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, player_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int seat = sqlite3_column_int(stmt, 0);
        auto pc = to_card_pair(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        auto bc = to_card_pair(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        auto board = deserialize_cards(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        auto history = deserialize_history(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));

        auto cards = (seat == 0)
            ? array<array<Card, 2>, 2>{{{pc[0], pc[1]}, {bc[0], bc[1]}}}
            : array<array<Card, 2>, 2>{{{bc[0], bc[1]}, {pc[0], pc[1]}}};
        PokerKit game = build_game(cards, history, board);
        auto stacks = game.get_stacks();
        total += (static_cast<int16_t>(stacks[seat]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
    }
    sqlite3_finalize(stmt);
    return total;
}

vector<PlayerStats> get_all_player_stats() {
    vector<PlayerStats> stats;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, username FROM players ORDER BY username", -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int player_id = sqlite3_column_int(stmt, 0);
        string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int hands = get_hand_count(player_id);
        float profit = get_total_profit(player_id);
        stats.push_back(PlayerStats{username, hands, profit});
    }
    sqlite3_finalize(stmt);
    return stats;
}
