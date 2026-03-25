#ifndef SERVER_RESPONSE_BUILDER_HPP
#define SERVER_RESPONSE_BUILDER_HPP

#include "crow.h"
#include <cstdint>
#include <string>
#include <vector>
#include "action.hpp"
#include "server_game_util.hpp"

crow::json::wvalue build_all_actions_json(bool is_preflop);

crow::json::wvalue build_valid_actions_json(
    const std::vector<Action>& valid_actions,
    bool is_preflop, bool is_check, uint16_t bet_to_face);

crow::json::wvalue build_past_hand_json(const HandData& past);

crow::json::wvalue build_state_response(HandData& hand);

#endif // SERVER_RESPONSE_BUILDER_HPP
