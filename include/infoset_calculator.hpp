#ifndef INFOSET_CALCULATOR_HPP
#define INFOSET_CALCULATOR_HPP

#include <string>
#include <vector>
#include "pokerkit.hpp"
#include "action.hpp"

class InfosetCalculator {
public:
    static std::string get_infoset_from_game(const PokerKit& game, uint8_t player, const std::vector<Action>& possible_actions);
};

#endif // INFOSET_CALCULATOR_HPP
