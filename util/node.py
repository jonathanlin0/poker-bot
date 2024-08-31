class Node:
    # min_regret_threshold is in terms of the prob of action
    # so the probability of any action is at least 0.5%
    # allows more coverage and exploration of game tree
    def __init__(self, actions):
        self.regret_sum = {a: 0 for a in actions}
        self.strat = {a: 0 for a in actions}
        self.strat_sum = {a: 0 for a in actions}
        self.actions = actions

    # can't use a lock for each loop, or else the strat may have changed from another thread, so the normalization won't be correct
    def calc_strat(self, hands_played):
        # calc initial normalizing sum
        temp_sum = sum(a for a in self.regret_sum.values() if a > 0)

        # calc min_regret based on function
        # function starts at 0.01 and decreases to 0% by 40,000 hands played
        min_regret_multiplier = max(0, -(2.5 * (10 ** (-7))) * hands_played + 0.01)

        # minimum regret threshold (so each action has at least min_regret_threshold probability)
        min_regret = min_regret_multiplier * temp_sum
        normalizing_sum = 0
        # set strat = regret_sum (neg vals r 0)
        for a in self.actions:
            if self.regret_sum[a] > min_regret:
            # if self.regret_sum[a] > 0:
                self.strat[a] = self.regret_sum[a]
            else:
                # self.strat[a] = 0
                self.strat[a] = min_regret
            normalizing_sum += self.strat[a]
        
        # normalizes the values in strat
        for a in self.actions:
            if normalizing_sum > 0:
                self.strat[a] /= normalizing_sum
            else:
                self.strat[a] = 1.0 / len(self.actions)
        
        return self.strat
    
    def get_avg_strat(self):
        # add strat to strat_sum once
        avg_strat = {a: 0 for a in self.actions}
        
        curr_strat_sum = self.strat_sum

        # add 1 set of regret_sum if all values of strat_sum are 0
        # having strat_sum = 0 just means that position hasn't been the nontraversing player yet. and we don't want uniform weights
        if sum(list(curr_strat_sum.values())) == 0:
            curr_strat_sum = self.regret_sum.copy()
            # set all neg values to 0
            for a in curr_strat_sum:
                if curr_strat_sum[a] < 0:
                    curr_strat_sum[a] = 0
        
        normalizing_sum = 0
        for a in self.actions:
            normalizing_sum += curr_strat_sum[a]
        for a in self.actions:
            if normalizing_sum > 0:
                avg_strat[a] = curr_strat_sum[a] / normalizing_sum
            else:
                avg_strat[a] = 1.0 / len(self.actions)
        
        return avg_strat
    