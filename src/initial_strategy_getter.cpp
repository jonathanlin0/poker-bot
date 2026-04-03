#include "../include/initial_strategy_getter.hpp"
#include "../include/config.hpp"
#include "../include/node.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;
using std::ifstream;
using std::once_flag;
using std::string;
using std::vector;
using std::runtime_error;
using std::getline;
using std::stoi;
using std::stof;
using std::call_once;

EquityMap InitialStrategyGetter::equities_{};
once_flag InitialStrategyGetter::load_flag_;
bool InitialStrategyGetter::use_precomputed_ = true;

EquityMap InitialStrategyGetter::load_precomputed_equities() {
    EquityMap data{};
    const string bucket_delimiter = " ";
    ifstream file(PRECOMPUTED_EQUITIES_FILE);
    if (!file.is_open()) {
        cout << "Warning: could not load precomputed equities from " << PRECOMPUTED_EQUITIES_FILE << endl;
        return data;
    }

    string infoset, wins_str, total_str;
    while (getline(file, infoset)) {
        if (!getline(file, wins_str) || !getline(file, total_str)) { break; }

        size_t first_delim = infoset.find(bucket_delimiter);
        size_t second_delim = infoset.find(bucket_delimiter, first_delim + 1);
        int street = stoi(infoset.substr(first_delim + 1, second_delim - first_delim - 1));

        data[street][infoset][0] += stof(wins_str);
        data[street][infoset][1] += stof(total_str);
    }

    size_t total = 0;
    for (int s = 0; s < 4; s++) total += data[s].size();
    cout << "Loaded precomputed equities: " << total << " infosets from " << PRECOMPUTED_EQUITIES_FILE << endl;
    return data;
}

const EquityMap& InitialStrategyGetter::get_equities() {
    ensure_loaded();
    return equities_;
}

void InitialStrategyGetter::set_use_precomputed_equities(bool use) {
    use_precomputed_ = use;
}

void InitialStrategyGetter::ensure_loaded() {
    // call_once guarantees the lambda func runs exactly once across all threads; subsequent calls are no-ops
    call_once(load_flag_, []() {
        if (use_precomputed_) {
            equities_ = load_precomputed_equities();
        }
    });
}

void InitialStrategyGetter::apply_equity_adjustments(vector<float>& distribution, const vector<Action>& valid_actions, float win_rate) {
    for (size_t i = 0; i < valid_actions.size(); i++) {
        char type = valid_actions[i].type;
        if (win_rate >= 0.9f) {
            if (type == 'f') { distribution[i] *= 0.2f; }
            else if (type == 'r') { distribution[i] *= 3.0f; }
            else if (type == 'a') { distribution[i] *= 4.0f; }
        } else if (win_rate >= 0.7f) {
            if (type == 'f') { distribution[i] *= 0.5f; }
            else if (type == 'r') { distribution[i] *= 2.0f; }
            else if (type == 'a') { distribution[i] *= 2.0f; }
        } else if (win_rate >= 0.4f) {
            if (type == 'c') { distribution[i] *= 3.0f; }
        } else {
            if (type == 'r') { distribution[i] *= 0.3f; }
            else if (type == 'a') { distribution[i] *= 0.3f; }
            else if (type == 'f') { distribution[i] *= 2.0f; }
        }
    }

    float normalizing_sum = 0;
    for (float val : distribution) {
        normalizing_sum += val;
    }
    if (normalizing_sum > 0) {
        for (float& val : distribution) {
            val /= normalizing_sum;
        }
    } else { // defensive programming
        throw runtime_error("apply_equity_adjustments: normalizing_sum is 0, this should never happen");
    }
}

vector<float> InitialStrategyGetter::get_initial_strategy(
    uint8_t street,
    const string& infoset,
    const vector<Action>& valid_actions
) {
    assert(!valid_actions.empty() && "valid_actions must have at least 1 element");
    ensure_loaded();
    
    float uniform = 1.0f / valid_actions.size();
    vector<float> probs(valid_actions.size(), uniform);

    string trimmed = trim_aggression_and_actions_off_infoset(infoset);
    auto it = equities_[street].find(trimmed);
    if (it != equities_[street].end() && it->second[1] > 0) {
        float win_rate = it->second[0] / it->second[1];
        apply_equity_adjustments(probs, valid_actions, win_rate);
    }

    return probs;
}