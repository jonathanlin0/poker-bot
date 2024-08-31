# this is a dictionary object except multithread friendly
# meaning that there is a lock for each key/value pair

import threading


class LockableDict:
    def __init__(self, reward_fn: str):
        self.reward_fn = reward_fn

        self.dict = {}
        self.locks = {}
        # for safely adding new locks
        self.dict_lock = threading.Lock()

    def keys(self):
        return self.dict.keys()

    def get(self, key):
        with self.get_lock(key):
            return self.dict.get(key)

    def set(self, key, value):
        with self.get_lock(key):
            self.dict[key] = value
    
    def get_regret_sum(self, infoset):
        with self.get_lock(infoset):
            return self.dict[infoset].regret_sum.copy()
    
    def get_strat_sum(self, infoset, action):
        with self.get_lock(infoset):
            return self.dict[infoset].strat_sum[action].copy()
    
    def get_strat(self, infoset, hands_played):
        with self.get_lock(infoset):
            return self.dict[infoset].calc_strat(hands_played).copy()
    
    def set_regret_sum(self, infoset, action, value):
        with self.get_lock(infoset):
            self.dict[infoset].regret_sum[action] = value

    def set_strat_sum(self, infoset, action, value):
        with self.get_lock(infoset):
            self.dict[infoset].strat_sum[action] = value

    def set_strat(self, infoset, action, value):
        with self.get_lock(infoset):
            self.dict[infoset].strat[action] = value
    
    def update_regret_sum(self, infoset, new_vals):
        # change the regret sum to put more weight on very negative values
        # so essentially make it risk adverse
        new_vals_adjusted = new_vals.copy()
        for key, val in new_vals.items():
            if val < 0:
                abs_val = abs(val)
                if self.reward_fn == "default":
                    pass
                elif self.reward_fn == "linear":
                    abs_val = 1.5 * abs_val
                elif self.reward_fn == "poly":
                    abs_val = 0.35 * (abs_val ** 1.32) + 5
                elif self.reward_fn == "default_exp":
                    abs_val = max(abs_val, 0.35 * (abs_val ** 1.32) + 5)
                else:
                    print(f"Reward function {self.reward_fn} not accounted for")
                    exit()

                new_vals_adjusted[key] = -abs_val

        with self.get_lock(infoset):
            for a in new_vals_adjusted:
                self.dict[infoset].regret_sum[a] += new_vals_adjusted[a]
    
    def update_strat_sum(self, infoset, new_vals):
        with self.get_lock(infoset):
            for a in new_vals:
                self.dict[infoset].strat_sum[a] += new_vals[a]

    def key_exists(self, key):
        with self.get_lock(key):
            return key in self.dict

    def get_lock(self, key):
        with self.dict_lock:
            if key not in self.locks:
                self.locks[key] = threading.Lock()
            return self.locks[key]