#ifndef INITIAL_STRATEGY_GETTER_HPP
#define INITIAL_STRATEGY_GETTER_HPP

#include "action.hpp"
#include "util.hpp"
#include <string>
#include <vector>
#include <mutex>

class InitialStrategyGetter {
    public:
    static std::vector<float> get_initial_strategy(
        uint8_t street,
        const std::string& infoset,
        const std::vector<Action>& valid_actions
    );

    /*
        Loads precomputed equity data from data/precomputed_equity.txt, separated by street.
        Returns an empty map and prints a warning if the file can't be loaded.
    */
    static const EquityMap& get_equities();

private:
    static EquityMap load_precomputed_equities();
    static void ensure_loaded();
    static void apply_equity_adjustments(std::vector<float>& distribution, const std::vector<Action>& valid_actions, float win_rate);
    static EquityMap equities_;
    // ensures equities_ is loaded exactly once, even if multiple threads call ensure_loaded() simultaneously
    static std::once_flag load_flag_;
};

#endif // INITIAL_STRATEGY_GETTER_HPP
