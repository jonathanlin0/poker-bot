#ifndef ACTION_HPP
#define ACTION_HPP

#include <cstdint>
#include <string>

struct Action {
    char type;         // 'c' = call/check, 'f' = fold, 'r' = raise, 'a' = all-in, 'b' = blind
    int16_t amount;    // chip amount (-1 for fold). Is the cumulative amount for a bet, INCLUDING the amount to call, if applicable.
    float pot_multiplier = 0;  // raise amount as a fraction of the pot (only used for 'r')
    
    Action(char type, int16_t amount, float pot_multiplier = 0.0f) : type(type), amount(amount), pot_multiplier(pot_multiplier) {}

    
    /*
        DEPRECATED
        Construct from string (e.g., "f", "c", "r10;0.50", "a")
    */
    [[deprecated("Use Action objects directly instead of round-tripping through strings")]]
    Action(const std::string& s) : type(s[0]), amount(0) {
        if (type == 'f') {
            amount = -1;
        }
        else if (type == 'r' && s.size() > 1) {
            size_t semi = s.find(';');
            amount = static_cast<int16_t>(std::stoi(s.substr(1, semi - 1)));
            if (semi != std::string::npos) { // ensure that a pot multiplier exists
                pot_multiplier = std::stof(s.substr(semi + 1));
            }
        }

        // TODO calculate amount for 'c' and 'a'
    }

    // Convert action to string (e.g., "f", "c", "r10;0.50", "a")
    // Allows: string(action) or static_cast<string>(action)
    explicit operator std::string() const {
        std::string result{type};
        if (type == 'r') {
            result += std::to_string(amount) + ";" + std::to_string(pot_multiplier);
        }
        return result;
    }
};

#endif // ACTION_HPP

