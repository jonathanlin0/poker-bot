#include "../include/server_game_util.hpp"
#include "crow.h"
#include <fstream>
#include <sstream>
#include <string>

using std::array;
using std::ifstream;
using std::ostringstream;
using std::string;
using std::to_string;
using std::vector;

string read_file(const string& path) {
    ifstream file(path);
    if (!file.is_open()) { return ""; }
    ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

bool is_valid_username(const string& username) {
    if (username.empty()) { return false; }
    for (char c : username) {
        if (!isalnum(c)) { return false; }
    }
    return true;
}

string card_to_str(const Card& c) {
    return string(1, c.suit) + string(1, c.rank);
}

array<array<Card, 2>, 2> get_cards(const HandData& hand) {
    if (hand.player_seat == 0) {
        return {{{hand.player_cards[0], hand.player_cards[1]},
                 {hand.bot_cards[0], hand.bot_cards[1]}}};
    } else {
        return {{{hand.bot_cards[0], hand.bot_cards[1]},
                 {hand.player_cards[0], hand.player_cards[1]}}};
    }
}

template<typename Container>
string serialize_cards(const Container& cards) {
    string r = "[";
    for (size_t i = 0; i < cards.size(); i++) {
        if (i) { r += ","; }
        r += "\"" + card_to_str(cards[i]) + "\"";
    }
    return r + "]";
}
template string serialize_cards(const array<Card, 2>&);
template string serialize_cards(const vector<Card>&);

string serialize_history(const vector<vector<Action>>& history) {
    string r = "[";
    for (size_t street = 0; street < history.size(); street++) {
        if (street) { r += ","; }
        r += "[";
        for (size_t action = 0; action < history[street].size(); action++) {
            if (action) { r += ","; }
            const auto& act = history[street][action];
            r += "{\"t\":\"" + string(1, act.type) + "\",\"a\":" + to_string(act.amount)
                 + ",\"p\":" + to_string(act.pot_multiplier) + "}";
        }
        r += "]";
    }
    return r + "]";
}

vector<Card> deserialize_cards(const string& json) {
    auto parsed = crow::json::load(json);
    vector<Card> cards;
    for (size_t i = 0; i < parsed.size(); i++) {
        string s = parsed[i].s();
        cards.push_back(Card(s[0], s[1]));
    }
    return cards;
}

vector<vector<Action>> deserialize_history(const string& json) {
    auto parsed = crow::json::load(json);
    vector<vector<Action>> history;
    for (size_t street = 0; street < parsed.size(); street++) {
        vector<Action> street_actions;
        for (size_t action = 0; action < parsed[street].size(); action++) {
            string type_str = parsed[street][action]["t"].s();
            char type = type_str[0];
            int16_t amount = static_cast<int16_t>(parsed[street][action]["a"].i());
            float pot_mult = static_cast<float>(parsed[street][action]["p"].d());
            street_actions.push_back(Action(type, amount, pot_mult));
        }
        history.push_back(street_actions);
    }
    return history;
}
