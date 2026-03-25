#include "crow.h"

#include "../include/server_game_util.hpp"
#include "../include/server_db_util.hpp"
#include "../include/server_response_builder.hpp"
#include "../include/pokerkit.hpp"
#include "../include/config.hpp"
#include "../include/initial_strategy_getter.hpp"
#include "../include/node.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/infoset_calculator.hpp"
#include "../include/serialization.hpp"
#include "../include/util.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <locale>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::fabs;
using std::lock_guard;
using std::mutex;
using std::string;
using std::unordered_map;
using std::vector;
using crow::request;
using crow::response;
namespace filesystem = std::filesystem;


const string EXPERIMENT_NAME = "default";
const string DB_PATH = "data/poker_data.sqlite3";
const string AVG_STRAT_PATH = "data/" + EXPERIMENT_NAME + "/avg_strat.bin";
const string STATIC_DIR = "frontend";
const int PORT = 9001;
const int NUM_SERVER_THREADS = 5;

array<unordered_map<string, Node>, 4> nodes;
mutex action_mutex;


response json_error(int code, const string& message) {
    crow::json::wvalue err;
    err["error"] = message;
    auto resp = response(code, err.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}


// Handles street transitions (dealing board cards) and all-in runouts.
// Sets hand.is_complete if the game ends.
void advance_game_state(HandData& hand) {
    if (is_game_over_from_history_vector(hand.history)) {
        hand.is_complete = true;
        return;
    }

    if (is_betting_street_done(hand.history.back())) {
        size_t num_streets = hand.history.size();
        if (num_streets == 1) {
            for (int i = 0; i < 3; i++) {
                hand.board.push_back(hand.deck.back());
                hand.deck.pop_back();
            }
        } else if (num_streets == 2 || num_streets == 3) {
            hand.board.push_back(hand.deck.back());
            hand.deck.pop_back();
        }
        hand.history.push_back({});
    }

    auto cards = get_cards(hand);
    PokerKit game = build_game(cards, hand.history, hand.board);
    while (!game.is_game_over() && !game.get_can_still_bet()) {
        hand.board.push_back(hand.deck.back());
        hand.deck.pop_back();
        hand.history.push_back({});
        game = build_game(cards, hand.history, hand.board);
    }

    if (game.is_game_over()) {
        hand.is_complete = true;
    }
}

// Looks up the bot's avg_strat for this infoset (falls back to equity-based initial strategy)
// and samples an action from the probability distribution.
Action choose_bot_action(const PokerKit& game, uint8_t bot_seat,
                         const vector<Action>& valid_actions) {
    uint8_t street = game.get_betting_street();
    string infoset = InfosetCalculator::get_infoset_from_game(game, bot_seat, valid_actions);

    unordered_map<string, float> strategy;
    if (nodes[street].count(infoset)) {
        strategy = nodes[street].at(infoset).get_avg_strat_map();
    } else {
        vector<float> initial = InitialStrategyGetter::get_initial_strategy(
            street, infoset, valid_actions);
        for (size_t i = 0; i < valid_actions.size(); i++) {
            strategy[string(valid_actions[i])] = initial[i];
        }
    }

    vector<float> probs;
    for (const Action& a : valid_actions) {
        auto it = strategy.find(string(a));
        probs.push_back(it != strategy.end() ? it->second : 0.0f);
    }

    float sum = 0.0f;
    for (float p : probs) sum += p;
    if (sum <= 0.0f) {
        float uniform = 1.0f / valid_actions.size();
        for (auto& p : probs) p = uniform;
    } else if (fabs(sum - 1.0f) > 1e-4f) {
        for (auto& p : probs) p /= sum;
    }

    size_t idx = sample_from_distribution_list(probs);
    return valid_actions[idx];
}

// Advances game state then repeatedly has the bot act until it's the player's turn or the hand ends.
void run_bot_loop(HandData& hand) {
    uint8_t bot_seat = 1 - hand.player_seat;

    advance_game_state(hand);

    while (!hand.is_complete) {
        auto cards = get_cards(hand);
        PokerKit game = build_game(cards, hand.history, hand.board);
        if (game.is_game_over()) {
            hand.is_complete = true;
            return;
        }

        uint8_t to_move = get_player_to_move(hand.history);
        if (to_move != bot_seat) return;

        bool is_preflop = (hand.history.size() == 1);
        vector<Action> valid = get_valid_actions(is_preflop, game);

        Action bot_action = choose_bot_action(game, bot_seat, valid);
        hand.history.back().push_back(bot_action);

        advance_game_state(hand);
    }
}


int main() {
    if (!filesystem::exists(AVG_STRAT_PATH)) {
        throw std::runtime_error("Weights file not found: " + AVG_STRAT_PATH);
    }

    cout.imbue(std::locale("en_US.UTF-8"));
    cout << "Loading avg_strat from " << AVG_STRAT_PATH << "..." << endl;
    load_avg_strat(AVG_STRAT_PATH, nodes);
    const string street_names[] = {"Preflop", "Flop", "Turn", "River"};
    size_t total_infosets = 0;
    for (int s = 0; s < 4; s++) {
        cout << "  " << street_names[s] << ": " << nodes[s].size() << " infosets" << endl;
        total_infosets += nodes[s].size();
    }
    cout << "Loaded " << total_infosets << " total infosets" << endl;

    InitialStrategyGetter::get_equities();

    filesystem::create_directories("data");
    int rc = sqlite3_open_v2(DB_PATH.c_str(), &db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + string(sqlite3_errmsg(db)));
    }
    init_db();
    cout << "Database ready at " << DB_PATH << endl;

    crow::SimpleApp app;

    // Home page
    CROW_ROUTE(app, "/")([] {
        auto html = read_file(STATIC_DIR + "/index.html");
        if (html.empty()) return response(404, "File not found");
        auto resp = response(html);
        resp.set_header("Content-Type", "text/html");
        return resp;
    });

    // Health check endpoint
    CROW_ROUTE(app, "/health")([] {
        crow::json::wvalue resp;
        resp["ok"] = true;
        return response(resp);
    });

    // Serves the login page
    CROW_ROUTE(app, "/play")([] {
        auto html = read_file(STATIC_DIR + "/login.html");
        if (html.empty()) return response(404, "File not found");
        auto resp = response(html);
        resp.set_header("Content-Type", "text/html");
        return resp;
    });

    // Upserts player, ensures a hand exists, and serves the game page
    CROW_ROUTE(app, "/play/<string>")([](const string& username) {
        if (!is_valid_username(username)) {
            return response(400, "Invalid username");
        }

        {
            lock_guard<mutex> lock(action_mutex);
            int player_id = upsert_player(username);
            auto maybe_hand = get_active_hand(player_id);
            if (!maybe_hand) {
                auto hand = create_hand(player_id);
                run_bot_loop(hand);
                update_hand_db(hand);
            }
        }

        auto html = read_file(STATIC_DIR + "/play.html");
        if (html.empty()) return response(404, "File not found");
        auto resp = response(html);
        resp.set_header("Content-Type", "text/html");
        return resp;
    });

    // Static CSS
    CROW_ROUTE(app, "/css/style.css")([] {
        auto css = read_file(STATIC_DIR + "/css/style.css");
        if (css.empty()) return response(404);
        auto resp = response(css);
        resp.set_header("Content-Type", "text/css");
        return resp;
    });

    // Static JS
    CROW_ROUTE(app, "/js/game.js")([] {
        auto js = read_file(STATIC_DIR + "/js/game.js");
        if (js.empty()) return response(404);
        auto resp = response(js);
        resp.set_header("Content-Type", "application/javascript");
        return resp;
    });

    // Returns the current hand state (or creates a new hand if none active)
    // hit on the play page and after an action is taken
    CROW_ROUTE(app, "/hand-state").methods("POST"_method)([](const request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("username")) {
            return json_error(400, "Missing username");
        }
        string username = body["username"].s();
        if (!is_valid_username(username)) {
            return json_error(400, "Username must be alphanumeric and non-empty");
        }

        lock_guard<mutex> lock(action_mutex);

        int player_id = find_player_id(username);
        if (player_id == -1) {
            return json_error(404, "Player not found");
        }

        auto maybe_hand = get_active_hand(player_id);
        if (!maybe_hand) {
            maybe_hand = create_hand(player_id);
            run_bot_loop(*maybe_hand);
            update_hand_db(*maybe_hand);
        }
        // for readability purposes, "maybe_hand" is renamed to "hand"
        HandData& hand = *maybe_hand;

        auto resp_json = build_state_response(hand);

        auto past = get_past_hands(player_id, 20);
        for (size_t i = 0; i < past.size(); i++) {
            resp_json["past_hands"][i] = build_past_hand_json(past[i]);
        }
        if (past.empty()) {
            resp_json["past_hands"] = vector<crow::json::wvalue>{};
        }

        float profit = get_total_profit(player_id);
        resp_json["user_profit_bb"] = profit;
        resp_json["bot_profit_bb"] = -profit;
        resp_json["hands_played"] = get_hand_count(player_id);

        return response(resp_json);
    });

    // Applies a player action, runs the bot's response, and returns updated state
    CROW_ROUTE(app, "/action").methods("POST"_method)([](const request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("username") || !body.has("action")) {
                return json_error(400, "Missing required fields");
            }

            string username = body["username"].s();
            if (!is_valid_username(username)) {
                return json_error(400, "Username must be alphanumeric and non-empty");
            }
            string action_type_str = body["action"]["type"].s();
            if (action_type_str.empty()) {
                return json_error(400, "Missing action type");
            }
            char action_type = action_type_str[0];

            lock_guard<mutex> lock(action_mutex);

            int player_id = find_player_id(username);
            if (player_id == -1) {
                return json_error(404, "Player not found");
            }

            auto maybe_hand = get_active_hand(player_id);
            if (!maybe_hand) {
                return json_error(400, "No active hand");
            }
            HandData& hand = *maybe_hand; // for readability purposes, "maybe_hand" is renamed to "hand"
            if (hand.is_complete) {
                return json_error(400, "Game is over");
            }

            auto cards = get_cards(hand);
            PokerKit game = build_game(cards, hand.history, hand.board);

            uint8_t to_move = get_player_to_move(hand.history);
            // defensive coding. should never happen
            if (to_move != static_cast<uint8_t>(hand.player_seat)) {
                return json_error(400, "Not your turn");
            }

            bool is_preflop = (hand.history.size() == 1);
            vector<Action> valid = get_valid_actions(is_preflop, game);

            const Action* validated_action = nullptr;
            if (action_type == 'f' || action_type == 'c' || action_type == 'a') {
                for (const auto& a : valid) {
                    if (a.type == action_type) { validated_action = &a; break; }
                }
            } else if (action_type == 'r') {
                if (!body["action"].has("amount")) {
                    return json_error(400, "Missing amount for raise");
                }
                // validate raise amount
                int increment = static_cast<int>(body["action"]["amount"].i());
                uint16_t raise_to = game.get_last_action().amount + static_cast<uint16_t>(increment);

                for (const auto& a : valid) {
                    if (a.type == 'r' && a.amount == raise_to) { validated_action = &a; break; }
                }
            }

            if (!validated_action) {
                return json_error(400, "Invalid action");
            }

            hand.history.back().push_back(*validated_action);

            run_bot_loop(hand);
            update_hand_db(hand);

            auto resp_json = build_state_response(hand);
            return response(resp_json);
        } catch (const std::exception& e) {
            cerr << "POST /action exception: " << e.what() << endl;
            return json_error(500, string("Internal error: ") + e.what());
        }
    });

    cout << "Starting server on port " << PORT << endl;
    app.port(PORT).concurrency(NUM_SERVER_THREADS).run();

    sqlite3_close(db);
    return 0;
}
