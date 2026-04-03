#include "../include/pokerkit.hpp"
#include "../include/action.hpp"
#include "../include/card.hpp"
#include "../include/config.hpp"
#include "../include/initial_strategy_getter.hpp"
#include "../include/node.hpp"
#include "../include/poker_game_util.hpp"
#include "../include/infoset_calculator.hpp"
#include "../include/serialization.hpp"
#include "../include/util.hpp"
#include "../include/validation.hpp"
#include "../include/cfr_util.hpp"
#include <atomic>
#include <iostream>
#include <array>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <cmath>

using std::array;
using std::atomic;
using std::ceil;
using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::lock_guard;
using std::max;
using std::mutex;
using std::ofstream;
using std::pair;
using std::runtime_error;
using std::stoi;
using std::stoll;
using std::string;
using std::thread;
using std::unordered_map;
using std::vector;
namespace chrono = std::chrono;
namespace filesystem = std::filesystem;

const int DEFAULT_EPOCHS = 2100000;
const int DEFAULT_NUM_THREADS = 4;

class Trainer {
public:
    Trainer(const string& experiment_name, int epochs, int num_threads, bool use_precomputed_equities);
    /*
      Loads the entire previous state of the experiment
    */
    void load_prev_data(int epochs_override, int num_threads_override);
    void train(); // formerly wrapper_cfr_iterations()

private:
    int epoch; // current epoch
    int epochs; // total number of epochs to train for
    int num_threads;
    string experiment_name;
    string experiment_dir;

    int next_epoch_to_calculate_exploitability;
    int next_epoch_to_perform_validation;

    // [street] -> { infoset_key -> Node }
    array<unordered_map<string, Node>, 4> nodes;
    array<mutex, 4> street_locks; // use via lock(street_locks[street]) for safe locking and unlocking. automatically unlocks when out of scope

    bool use_precomputed_equities;
    atomic<long long> total_hands_played; // thread safe type
    mutex stats_lock;
    unordered_map<string, float> infoset_to_hands_played;
    double interval_regret_sum;

    void ensure_node_exists(uint8_t street, const string& infoset, const vector<Action>& valid_actions);
    unordered_map<string, float> get_strat(uint8_t street, const string& infoset, const vector<Action>& valid_actions);
    void update_regret_sum(uint8_t street, const string& infoset, const unordered_map<string, float>& regret);
    void update_strat_sum(uint8_t street, const string& infoset, const unordered_map<string, float>& strat, const vector<Action>& valid_actions, int current_epoch);
    static pair<float, float> get_regret(const PokerKit& game);
    float external_cfr(uint8_t traversing_player, const array<array<Card, 2>, 2>& cards, vector<vector<Action>> all_history, vector<Card> board, vector<Card> deck, int current_epoch);

    /*
        Saves the config and metadata snapshot of the current experiment.
        Config is essentially the Trainer object's fields.
        Used to resume paused training and plotting scripts.
    */
    void save_metadata();
    /*
      Loads the config/metadata snapshot of the previous experiment.
      Note: this excludes nodes
    */
    void load_metadata();
    // refreshes the nodes' strat values (derived from regret_sum). used when loading in previous weights.
    void recalculate_strategies();
};


// TODO: remove the magic numbers in this constructor
Trainer::Trainer(const string& experiment_name, int epochs, int num_threads, bool use_precomputed_equities)
    : epoch(0),
      epochs(epochs),
      num_threads(num_threads),
      experiment_name(experiment_name),
      experiment_dir("data/" + experiment_name),
      next_epoch_to_calculate_exploitability(50000),
      next_epoch_to_perform_validation(5000),
      use_precomputed_equities(use_precomputed_equities),
      total_hands_played(0),
      interval_regret_sum(0.0)
{
    InitialStrategyGetter::set_use_precomputed_equities(use_precomputed_equities);
}

void Trainer::load_prev_data(int epochs_override, int num_threads_override) {
    load_metadata();
    if (epochs_override != -1) { epochs = epochs_override; }
    if (num_threads_override != -1) { num_threads = num_threads_override; }
    cout << "Loading weights from " << experiment_dir << "/weights.bin" << endl;
    load_nodes(experiment_dir + "/weights.bin", nodes);
    recalculate_strategies();
    cout << "Resuming training from epoch " << epoch << " (target: " << epochs << " total epochs)" << endl;
}


void Trainer::save_metadata() {
    ofstream file(experiment_dir + "/metadata.txt");
    if (!file.is_open()) {
        throw runtime_error("Failed to open file: " + experiment_dir + "/metadata.txt");
    }
    file << "epoch:" << epoch << "\n";
    file << "epochs:" << epochs << "\n";
    file << "num-threads:" << num_threads << "\n";
    file << "next-epoch-to-calculate-exploitability:" << next_epoch_to_calculate_exploitability << "\n";
    file << "next-epoch-to-perform-validation:" << next_epoch_to_perform_validation << "\n";
    file << "total-hands-played:" << total_hands_played.load() << "\n";
    file << "pre-eq:" << (use_precomputed_equities ? 1 : 0) << "\n";
}

void Trainer::load_metadata() {
    string path = experiment_dir + "/metadata.txt";
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Failed to open metadata file: " + path + " (does the experiment exist?)");
    }
    string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) { continue; }
        string key = line.substr(0, colon);
        string value = line.substr(colon + 1);
        if (key == "epoch") { epoch = stoi(value); }
        else if (key == "epochs") { epochs = stoi(value); }
        else if (key == "num-threads") { num_threads = stoi(value); }
        else if (key == "next-epoch-to-calculate-exploitability") { next_epoch_to_calculate_exploitability = stoi(value); }
        else if (key == "next-epoch-to-perform-validation") { next_epoch_to_perform_validation = stoi(value); }
        else if (key == "total-hands-played") { total_hands_played = stoll(value); }
        else if (key == "pre-eq") { use_precomputed_equities = (stoi(value) != 0); } // boolean converted to int in save_metadata()
    }
    cout << "Loaded metadata:" << endl;
    cout << "  epoch=" << epoch << endl;
    cout << "  epochs=" << epochs << endl;
    cout << "  num_threads=" << num_threads << endl;
    cout << "  next_exploitability=" << next_epoch_to_calculate_exploitability << endl;
    cout << "  next_validation=" << next_epoch_to_perform_validation << endl;
    cout << "  total_hands_played=" << total_hands_played << endl;
    cout << "  pre_eq=" << use_precomputed_equities << endl;
}

void Trainer::recalculate_strategies() {
    for (int street = 0; street < 4; street++) {
        for (auto& [infoset, node] : nodes[street]) {
            // node already exists from loaded weights; dummy actions just need correct size
            // dummy nodes are never used, since the infoset is already created from load_nodes
            vector<Action> valid_actions(node.actions.size(), Action('c', 0));
            auto strat_map = get_strat(street, infoset, valid_actions);

            for (size_t i = 0; i < node.actions.size(); i++) {
                node.strat[i] = strat_map[node.actions[i]];
            }
        }
    }
    calculate_avg_strat(nodes);
}


/*
    Ensures that a node exists for the given infoset and street.
    Populates it with default values if it doesn't exist.
*/
void Trainer::ensure_node_exists(uint8_t street, const string& infoset, const vector<Action>& valid_actions) {
    if (nodes[street].find(infoset) == nodes[street].end()) {
        nodes[street][infoset] = Node(street, infoset, valid_actions);
    }
}

// Returns strategy (normalized positive regrets) for an infoset
unordered_map<string, float> Trainer::get_strat(uint8_t street, const string& infoset, const vector<Action>& valid_actions) {
    ensure_node_exists(street, infoset, valid_actions);
    const Node& node = nodes[street][infoset];
    unordered_map<string, float> strategy;

    // calculate total regret pre transformations
    float total_regret = 0.0f;
    for (size_t i = 0; i < node.actions.size(); i++) {
        total_regret += max(0.0f, node.regret_sum[i]); // regret_sum[i] should always be nonnegative in this version. but clamping it in case changed in the future
    }

    // apply minimum regret sum for regularization purposes
    for (size_t i = 0; i < node.actions.size(); i++) {
        if (total_regret == 0.0f) {
            strategy[node.actions[i]] = 1.0f / valid_actions.size();
        } 
        else {
        //     float uniform_prob = total_regret / valid_actions.size();
        //     float uniform_weight = max(0.0f, (15.0f - infoset_to_hands_played[infoset]) / 100.0f);
        //     strategy[node.actions[i]] = (uniform_prob * uniform_weight) + std::max(0.0f, node.regret_sum[i]); 
            strategy[node.actions[i]] = max(0.0f, node.regret_sum[i]); // regret_sum[i] should always be nonnegative in this version. but clamping it in case changed in the future
        }
    }

    // re-calculate total regret post transformations
    total_regret = 0.0f;
    for (const auto& [action, regret] : strategy) {
        total_regret += regret;
    }

    // normalize strategy
    for (const auto& [action, prob] : strategy) {
        strategy[action] /= total_regret;
    }
    
    return strategy;
}

void Trainer::update_regret_sum(
    uint8_t street,
    const string& infoset,
    const unordered_map<string, float>& regret
) {
    Node& node = nodes[street][infoset];
    for (const auto& [action, val] : regret) {
        size_t idx = node.action_index(action);
        node.regret_sum[idx] = max(0.0f, node.regret_sum[idx] + val); // clamp the cum_regret to nonnegative for faster convergence. in vanilla CFR, we DON'T do this.
    }
}

void Trainer::update_strat_sum(uint8_t street, const string& infoset, const unordered_map<string, float>& strat, const vector<Action>& valid_actions, int current_epoch) {
    // the first AVERAGING_DELAY epochs r treated as warm up, so they aren't considered for updating the strat sum
    int weight = max(0, current_epoch - AVERAGING_DELAY);
    if (weight == 0) { return; }
    ensure_node_exists(street, infoset, valid_actions);
    Node& node = nodes[street][infoset];
    for (const auto& [action, prob] : strat) {
        size_t idx = node.action_index(action);
        node.strat_sum[idx] += prob * weight; // iteration based weighting -> later iterations have more weight since they're considered more influential and representative of the optimal game strat
    }
}

// Returns the payoff for each player given hole cards and full betting history
// Payoffs are in BB (starting stack delta / 2)
pair<float, float> Trainer::get_regret(const PokerKit& game) {
    if (!game.is_game_over()) {
        throw runtime_error("Game is not over. " + game.debug_print_history());
    }

    auto stacks = game.get_stacks();
    return {
        (static_cast<int16_t>(stacks[0]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f,
        (static_cast<int16_t>(stacks[1]) - static_cast<int16_t>(STARTING_STACK)) / 2.0f
    };
}


/*
    This is the main function that implements the External CFR algorithm.
    Returns the regret for the traversing player.
*/
float Trainer::external_cfr(
    uint8_t traversing_player,
    const array<array<Card, 2>, 2>& cards,
    vector<vector<Action>> all_history,
    vector<Card> board,
    vector<Card> deck,
    int current_epoch
) {
    PokerKit game = build_game(cards, all_history, board);

    if (is_game_over_from_history_vector(all_history)) {
        pair<float, float> regret = get_regret(game);
        total_hands_played++;
        return traversing_player == 0 ? regret.first : regret.second;
    }

    if (is_betting_street_done(all_history.back())) {
        if (all_history.size() == 1) {
            board.push_back(deck.back());
            deck.pop_back();
            board.push_back(deck.back());
            deck.pop_back();
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
        }
        else if (all_history.size() == 2 || all_history.size() == 3) {
            board.push_back(deck.back());
            deck.pop_back();
            all_history.push_back({});
        }
        // don't have to consider when history size == 4, otherwise hand would be done

        // Rebuild game with the new board cards and street
        game = build_game(cards, all_history, board);
    }

    // Handle all-in runout: deal remaining cards when no more betting is possible
    // don't have to deal with preflop shove, cause previous if/else handles it and deals appropriately
    while (!game.is_game_over() && !game.get_can_still_bet()) {
        board.push_back(deck.back());
        deck.pop_back();
        all_history.push_back({});
        game = build_game(cards, all_history, board); // refresh game. TODO: make this more time efficient in the future
    }

    if (game.is_game_over()) {
        pair<float, float> regret = get_regret(game);
        total_hands_played++;
        return traversing_player == 0 ? regret.first : regret.second;
    }

    uint8_t player = get_player_to_move(all_history);

    // define the possible actions
    bool is_preflop = (all_history.size() == 1);
    vector<Action> valid_actions = get_valid_actions(is_preflop, game);

    uint8_t street = game.get_betting_street();

    string infoset = InfosetCalculator::get_infoset_from_game(game, player, valid_actions);


    // called up here since they're used by both traversing and sampled player
    unordered_map<string, float> strategy;
    {
        lock_guard<mutex> lock(street_locks[street]);
        ensure_node_exists(street, infoset, valid_actions);
        strategy = get_strat(street, infoset, valid_actions);
    }

    if (player == traversing_player) {
        unordered_map<string, float> action_util;
        for (const Action& a : valid_actions) {
            action_util[string(a)] = 0.0f;
        }

        float node_util = 0.0f;
        for (const Action& a : valid_actions) {
            all_history.back().push_back(a);

            action_util[string(a)] = external_cfr(traversing_player, cards, all_history, board, deck, current_epoch);
            node_util += strategy[string(a)] * action_util[string(a)];
            all_history.back().pop_back();
        }

        unordered_map<string, float> regret;
        for (const Action& a : valid_actions) {
            regret[string(a)] = action_util[string(a)] - node_util;
        }

        {
            lock_guard<mutex> lock(street_locks[street]);
            update_regret_sum(street, infoset, regret);
        }
        {
            lock_guard<mutex> lock(stats_lock);
            // TEMP: track the positive clamped regret sum
            for (const auto& [action, val] : regret) {
                interval_regret_sum += max(0.0, static_cast<double>(val));
            }

            // TEMP
            // maybe in future have this tied to a separate mutex lock, not the same one as the nodes
            infoset_to_hands_played[infoset]++;
        }

        return node_util;
    }
    else { // acting player != traversing player
        vector<float> probs;
        for (const Action& a : valid_actions) {
            probs.push_back(strategy[string(a)]);
        }
        size_t idx = sample_from_distribution_list(probs);
        all_history.back().push_back(valid_actions[idx]);

        float util = external_cfr(
            traversing_player,
            cards,
            all_history,
            board,
            deck,
            current_epoch
        );

        {
            lock_guard<mutex> lock(street_locks[street]);
            update_strat_sum(street, infoset, strategy, valid_actions, current_epoch);
        }
        return util;
    }

    // should not get here    
    return -1; // will wrap around lol
}


void Trainer::train() {
    const EquityMap& precomputed_equities = InitialStrategyGetter::get_equities();

    // Clear experiment directory if starting from scratch
    if (epoch == 0) {
        filesystem::remove_all(experiment_dir);
    }
    filesystem::create_directories(experiment_dir);
    filesystem::create_directories(experiment_dir + "/variant_play");

    save_metadata();

    for (int i = epoch; i < epochs; i++) {
        // housekeeping logic
        if (i > 0 && i % VALIDATION_INTERVAL == 0) {
            cout << "Epoch " << i << " completed (" << total_hands_played << " hands played)" << endl;
            
            // TEMP: saves the change in regret sum for
            if (i > 0) {
                ofstream regret_file(experiment_dir + "/regret.txt", ios::app);
                if (!regret_file.is_open()) {
                    throw runtime_error("Failed to open file: " + experiment_dir + "/regret.txt");
                }
                regret_file << i << "\n" << interval_regret_sum << "\n";
                regret_file.close();
                // reset the interval regret sum
                interval_regret_sum = 0.0;
            }

            epoch = i;
            save_metadata();
        }

        if (i == next_epoch_to_calculate_exploitability) {
            cout << "Computing exploitability at epoch " << i << "..." << endl;
            auto exploit_start = chrono::steady_clock::now();
            float exploitability = Validation::compute_exploitability(nodes, precomputed_equities);
            double exploit_secs = chrono::duration<double>(chrono::steady_clock::now() - exploit_start).count();
            cout << "Exploitability: (" << exploit_secs << "s)" << endl;
            string exploit_path = experiment_dir + "/exploitability.txt";
            ofstream exploit_file(exploit_path, ios::app);
            if (!exploit_file.is_open()) {
                throw runtime_error("Failed to open file: " + exploit_path);
            }
            exploit_file << i << "\n" << exploitability << "\n";
            exploit_file.close();
            next_epoch_to_calculate_exploitability = ceil(next_epoch_to_calculate_exploitability * 1.5);

            epoch = i;
            save_metadata();
        }

        if (i == next_epoch_to_perform_validation) {
            cout << "Performing validation at epoch " << i << endl;
            calculate_avg_strat(nodes);
            Validation::play_variants(experiment_dir, i, VARIANT_NAMES, nodes, infoset_to_hands_played, precomputed_equities);
            next_epoch_to_perform_validation = ceil((next_epoch_to_perform_validation != 0 ? next_epoch_to_perform_validation : 1) * 1.3);

            epoch = i;
            save_metadata();
        }

        // save nodes to disk
        if (i > 0 && i % SAVE_INTERVAL == 0) {
            cout << "Saving nodes at epoch " << i << "..." << endl;
            calculate_avg_strat(nodes);
            save_nodes(experiment_dir, nodes);

            epoch = i;
            save_metadata();
        }
        

        // actual CFR logic
        auto run_traversal = [this, i](uint8_t traversing_player) {
            // Create a new shuffled deck
            vector<Card> deck = get_new_deck(true);
            
            // Draw 2 cards for each player from the back of the deck
            Card c0 = deck.back(); deck.pop_back();
            Card c1 = deck.back(); deck.pop_back();
            Card c2 = deck.back(); deck.pop_back();
            Card c3 = deck.back(); deck.pop_back();
            array<array<Card, 2>, 2> cards = {{{c0, c1}, {c2, c3}}};
            
            // Initialize empty history (start with one empty street for preflop)
            vector<vector<Action>> all_history = {{}};
            
            // Initialize empty board
            vector<Card> board;
            
            external_cfr(traversing_player, cards, all_history, board, deck, i);
        };

        vector<thread> threads;
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back(run_traversal, 0);
            threads.emplace_back(run_traversal, 1);
        }
        for (auto& th : threads) {
            th.join();
        }
    }

    cout << "Node keys per street:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "  Street " << i << ": " << nodes[i].size() << " infosets" << endl;
    }
}


int main(int argc, char* argv[]) {
    string experiment_name = "default";
    int num_threads = -1;
    int epochs = -1;
    bool continue_training = false;
    bool use_precomputed_equities = true;

    /*
      --continue flag indicates to resume training from the last saved state for the given experiment.
      If --continue is NOT provided, then the other flags are used to set the initial parameters (default values used if none provided).
      If --continue IS provided, then providing the flags will override the current saved parameters. If the flags are NOT provided, then the previous saved parameters are used.
    */
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-n" && i + 1 < argc) {
            experiment_name = argv[++i];
        } else if (string(argv[i]) == "--num-threads" && i + 1 < argc) {
            num_threads = stoi(argv[++i]);
            if (num_threads < 1) {
                cerr << "Error: --num-threads must be a positive integer" << endl;
                return 1;
            }
        } else if (string(argv[i]) == "--epochs" && i + 1 < argc) {
            epochs = stoi(argv[++i]);
            if (epochs < 1) {
                cerr << "Error: --epochs must be a positive integer" << endl;
                return 1;
            }
        } else if (string(argv[i]) == "--continue") {
            continue_training = true;
        } else if (string(argv[i]) == "--no-pre-eq") {
            use_precomputed_equities = false;
        } else {
            cerr << "Usage: " << argv[0] << " -n <name> [--num-threads <N>] [--epochs <N>] [--continue] [--no-pre-eq]" << endl;
            return 1;
        }
    }

    if (continue_training) {
        // load previous state; explicit command line arguments override saved values
        Trainer trainer(experiment_name, DEFAULT_EPOCHS, DEFAULT_NUM_THREADS, use_precomputed_equities);
        trainer.load_prev_data(epochs, num_threads);
        trainer.train();
    } else {
        Trainer trainer(experiment_name,
                        epochs != -1 ? epochs : DEFAULT_EPOCHS,
                        num_threads != -1 ? num_threads : DEFAULT_NUM_THREADS,
                        use_precomputed_equities);
        trainer.train();
    }

    return 0;
}
