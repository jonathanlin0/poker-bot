#include "../include/server_response_builder.hpp"
#include "../include/config.hpp"
#include "../include/pokerkit.hpp"
#include "../include/poker_game_util.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using std::ostringstream;
using std::string;
using std::to_string;
using std::vector;

static string format_pot_mult(float v) {
    ostringstream oss;
    oss << std::fixed << std::setprecision(1) << v;
    return oss.str();
}

static void write_history_json(crow::json::wvalue& dest, const string& key, const vector<vector<Action>>& history) {
    for (size_t street = 0; street < history.size(); street++) {
        int idx = 0;
        if (street == 0) {
            dest[key][street][idx]["type"] = "b";
            dest[key][street][idx]["amount"] = 1;
            idx++;
            dest[key][street][idx]["type"] = "b";
            dest[key][street][idx]["amount"] = 2;
            idx++;
        }
        for (size_t action = 0; action < history[street].size(); action++) {
            dest[key][street][idx]["type"] = string(1, history[street][action].type);
            dest[key][street][idx]["amount"] = static_cast<int>(history[street][action].amount);
            if (history[street][action].type == 'r') {
                dest[key][street][idx]["pot_multiplier"] = history[street][action].pot_multiplier;
            }
            idx++;
        }
        if (idx == 0) {
            dest[key][street] = vector<crow::json::wvalue>{};
        }
    }
}

crow::json::wvalue build_all_actions_json(bool is_preflop) {
    crow::json::wvalue arr;
    int idx = 0;
    arr[idx++] = "f";
    arr[idx++] = "ch";
    arr[idx++] = "c";
    if (is_preflop) {
        for (uint8_t bb : PREFLOP_RAISE_SIZES) {
            arr[idx++] = to_string(static_cast<int>(bb));
        }
    } else {
        for (float mult : POSTFLOP_RAISE_SIZES) {
            arr[idx++] = format_pot_mult(mult);
        }
    }
    arr[idx++] = "a";
    return arr;
}

crow::json::wvalue build_valid_actions_json(
    const vector<Action>& valid_actions,
    bool is_preflop, bool is_check, uint16_t bet_to_face
) {
    crow::json::wvalue arr;
    int idx = 0;

    for (const Action& a : valid_actions) {
        crow::json::wvalue obj;

        if (a.type == 'f') {
            obj["label"] = "f";
            obj["type"] = "f";
        } else if (a.type == 'c') {
            obj["label"] = is_check ? "ch" : "c";
            obj["type"] = "c";
        } else if (a.type == 'r') {
            uint16_t increment = a.amount - bet_to_face;
            if (is_preflop) {
                obj["label"] = to_string(static_cast<int>(increment / 2));
            } else {
                obj["label"] = format_pot_mult(a.pot_multiplier);
            }
            obj["type"] = "r";
            obj["amount"] = static_cast<int>(increment);
        } else if (a.type == 'a') {
            obj["label"] = "a";
            obj["type"] = "a";
        }

        arr[idx++] = std::move(obj);
    }
    return arr;
}

crow::json::wvalue build_past_hand_json(const HandData& past) {
    auto cards = get_cards(past);
    PokerKit game = build_game(cards, past.history, past.board);
    auto stacks = game.get_stacks();
    float result_bb = (static_cast<int16_t>(stacks[past.player_seat])
                       - static_cast<int16_t>(STARTING_STACK)) / 2.0f;

    crow::json::wvalue h;
    h["hand_id"] = past.hand_id;
    h["player_seat"] = past.player_seat;
    h["result_bb"] = result_bb;

    h["player_cards"][0] = card_to_str(past.player_cards[0]);
    h["player_cards"][1] = card_to_str(past.player_cards[1]);
    h["bot_cards"][0] = card_to_str(past.bot_cards[0]);
    h["bot_cards"][1] = card_to_str(past.bot_cards[1]);

    for (size_t i = 0; i < past.board.size(); i++) {
        h["board"][i] = card_to_str(past.board[i]);
    }
    if (past.board.empty()) {
        h["board"] = vector<string>{};
    }

    write_history_json(h, "history", past.history);

    return h;
}

crow::json::wvalue build_state_response(HandData& hand) {
    auto cards = get_cards(hand);
    PokerKit game = build_game(cards, hand.history, hand.board);
    auto stacks = game.get_stacks();

    crow::json::wvalue resp;
    resp["player_stack"] = stacks[hand.player_seat] / 2.0;
    resp["bot_stack"] = stacks[1 - hand.player_seat] / 2.0;
    resp["pot"] = game.get_pot_size() / 2.0;
    resp["game_over"] = hand.is_complete;
    resp["player_seat"] = hand.player_seat;
    resp["hand_id"] = hand.hand_id;

    resp["player_cards"][0] = card_to_str(hand.player_cards[0]);
    resp["player_cards"][1] = card_to_str(hand.player_cards[1]);

    if (hand.is_complete) {
        resp["bot_cards"][0] = card_to_str(hand.bot_cards[0]);
        resp["bot_cards"][1] = card_to_str(hand.bot_cards[1]);

        float result_bb = (static_cast<int16_t>(stacks[hand.player_seat])
                           - static_cast<int16_t>(STARTING_STACK)) / 2.0f;
        resp["result_bb"] = result_bb;
    }

    for (size_t i = 0; i < hand.board.size(); i++) {
        resp["board"][i] = card_to_str(hand.board[i]);
    }
    if (hand.board.empty()) {
        resp["board"] = vector<string>{};
    }

    if (!hand.is_complete) {
        bool is_preflop = (hand.history.size() == 1);
        vector<Action> valid = get_valid_actions(is_preflop, game);

        uint16_t bet_to_face = game.get_last_action().amount;
        uint16_t player_bet = game.get_player_current_raise(
            static_cast<uint8_t>(hand.player_seat));
        bool is_check = (bet_to_face <= player_bet);

        resp["all_actions"] = build_all_actions_json(is_preflop);
        resp["valid_actions"] = build_valid_actions_json(
            valid, is_preflop, is_check, bet_to_face);
        resp["is_check"] = is_check;
    } else {
        resp["all_actions"] = vector<string>{};
        resp["valid_actions"] = vector<crow::json::wvalue>{};
    }

    write_history_json(resp, "history", hand.history);

    return resp;
}
