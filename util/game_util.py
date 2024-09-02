from pokerkit import Automation, NoLimitTexasHoldem
from util.game_setup import GameSetup
from math import floor, ceil

class GameUtil:
    """
    Functions that help manipulate and run the game.
    Aren't specific to any models or training algorithms
    """
    # dict mapping an index to the raise values after (not including the index)
    RAISE_VALS_PREFLOP_AFTER = {}
    RAISE_VALS_POSTFLOP_AFTER = {}

    for i in range(len(GameSetup.get_raise_vals_preflop())):
        RAISE_VALS_PREFLOP_AFTER[i] = GameSetup.get_raise_vals_preflop()[i + 1:]
    for i in range(len(GameSetup.get_raise_vals_postflop())):
        RAISE_VALS_POSTFLOP_AFTER[i] = GameSetup.get_raise_vals_postflop()[i + 1:]

    preflop_bet_to_index = {}
    for i, bet in enumerate(GameSetup.get_raise_vals_preflop()):
        preflop_bet_to_index[bet] = i
    postflop_bet_to_index = {}
    for i, bet in enumerate(GameSetup.get_raise_vals_postflop()):
        postflop_bet_to_index[bet] = i

    @staticmethod
    def get_player_curr(history):
        """
        returns the player that NEEDS TO MOVE
        """
        return 0 if (len(history) % 2) == 0 else 1

    @staticmethod
    # returns pot size and stack sizes
    def get_game_stats(betting_history, STARTING_STACK_SIZE=100):
        betting_history = betting_history.copy()
        assert len(betting_history) > 0

        state = NoLimitTexasHoldem.create_state(
            (
                Automation.ANTE_POSTING,
                Automation.BET_COLLECTION,
                Automation.BLIND_OR_STRADDLE_POSTING,
                Automation.CARD_BURNING,
                Automation.HOLE_CARDS_SHOWING_OR_MUCKING,
                Automation.HAND_KILLING,
                Automation.CHIPS_PUSHING,
                Automation.CHIPS_PULLING,
            ),
            False,
            0, # antes
            (1, 2), # blinds
            1, # min bet
            (STARTING_STACK_SIZE * 2, STARTING_STACK_SIZE * 2), # starting stacks
            2, # num of players
        )

        state.deal_hole(2)
        state.deal_hole(2)

        # preflop
        for i, bet in enumerate(betting_history[0]):
            if i == 0 or i == 1:
                continue
            else:
                if bet == "f":
                    state.fold()
                elif bet == "c" or bet == "ch":
                    state.check_or_call()
                elif bet == "a":
                    state.complete_bet_or_raise_to(STARTING_STACK_SIZE * 2)
                else:
                    # multiply by 2 because preflop betting is in units of BB, and game is in units of SB
                    state.complete_bet_or_raise_to(int(bet) * 2)
        
        if "a" in betting_history[0] and "f" not in betting_history[0] and betting_history[0][-1] != "a":
            state.deal_board(3)
            state.deal_board(1)
            state.deal_board(1)
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            # switch SB and BB returns for preflop
            # NOTE: don't acc need to reverse the stack outputs cuz get_regret is only run when the game is finished
            return {
                "pot_size": state.total_pot_amount / 2,
                "stack_sizes": [(state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2]
            }
        # no more betting
        # last_completed_round += 1
        # only gets here if someone folded
        if len(betting_history) == 1:
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            # switch SB and BB returns for preflop
            return {
                "pot_size": state.total_pot_amount / 2,
                "stack_sizes": [(state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2]
            }

        # flop
        stacks_before_round = state.stacks.copy()
        state.deal_board(3)
        for bet in betting_history[1]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(betting_history[1])]
                money_in = stacks_before_round[GameUtil.get_player_curr(betting_history[1])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(betting_history[1])])
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        # check if all in occured
        if "a" in betting_history[1] and "f" not in betting_history[1] and betting_history[1][-1] != "a":
            # state.deal_board(board[3])
            # state.deal_board(board[4])
            state.deal_board(1)
            state.deal_board(1)
            # state.deal_board(1)
            # state.deal_board(1)
        if len(betting_history) == 2:
            return {
                "pot_size": state.total_pot_amount / 2,
                "stack_sizes": [(state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2]
            }

        # turn
        stacks_before_round = state.stacks.copy()
        state.deal_board(1)
        for bet in betting_history[2]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(betting_history[2])]
                money_in = stacks_before_round[GameUtil.get_player_curr(betting_history[2])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(betting_history[2])])
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        # check if all in occured
        if "a" in betting_history[2] and "f" not in betting_history[2] and betting_history[2][-1] != "a":
            # state.deal_board(board[4])
            state.deal_board(1)
        if len(betting_history) == 3:
            return {
                "pot_size": state.total_pot_amount / 2,
                "stack_sizes": [(state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2]
            }
        
        # river
        stacks_before_round = state.stacks.copy()
        state.deal_board(1)
        for bet in betting_history[3]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(betting_history[3])]
                money_in = stacks_before_round[GameUtil.get_player_curr(betting_history[3])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(betting_history[3])])
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        return {
            "pot_size": state.total_pot_amount / 2,
            "stack_sizes": [(state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2]
        }

    @staticmethod
    def is_done_betting_preflop(history):
        """
        Returns true if the betting is done for the preflop round.
        """
        # a player folds
        if history[-1] == "f":
            return True
        # a player calls, and it isn't the BB first move
        # if history[-1] == "c" and len(history.split(",")) != 3:
        if history[-1] == "c" and len(history) != 3:
            return True
        # a player checks (only possible for the BB to do on the first move)
        # will need to change this once we add in the other streets: flop, turn, and river
        # if history[-2:] == "ch":
        if history[-1] == "ch":
            return True
        # someone went all in and a call occured afterward
        if "a" in history:
            idx = history.index("a")
            if idx < len(history) - 2:
                if "c" in history[idx + 1:]:
                    return True
        return False

    @staticmethod
    def is_done_betting_postflop(history):
        """
        Returns true if the betting is done for the postflop round.
        """
        if len(history) == 0:
            return False
        # a player folds or calls
        if history[-1] == "f" or history[-1] == "c":
            return True
        # two checks
        if len(history) == 2 and history[-1] == "ch":
            return True
        # someone went all in and a call occured afterward
        if "a" in history and "c" in history:
            return True
        return False

    @staticmethod
    def get_regret(cards, all_history, board, STARTING_STACK_SIZE=100):
        """
        Returns the regret for each player in the game.
        Regret is simply the net profit/loss of each player
        """
        last_completed_round = 0
        assert(len(all_history) >= 1)
        # check that the game is actually done
        assert GameUtil.is_done_betting_preflop(all_history[0])
        if len(all_history) > 1:
            assert GameUtil.is_done_betting_postflop(all_history[1])
        if len(all_history) > 2:
            assert GameUtil.is_done_betting_postflop(all_history[2])
        if len(all_history) > 3:
            assert GameUtil.is_done_betting_postflop(all_history[3])
        
        # simulate a game
        # remember that preflop betting is in units of BB
        # postflop betting is in percentage of pot
        state = NoLimitTexasHoldem.create_state(
            (
                Automation.ANTE_POSTING,
                Automation.BET_COLLECTION,
                Automation.BLIND_OR_STRADDLE_POSTING,
                Automation.CARD_BURNING,
                Automation.HOLE_CARDS_SHOWING_OR_MUCKING,
                Automation.HAND_KILLING,
                Automation.CHIPS_PUSHING,
                Automation.CHIPS_PULLING,
            ),
            False,
            0, # antes
            (1, 2), # blinds
            1, # min bet
            (STARTING_STACK_SIZE * 2, STARTING_STACK_SIZE * 2), # starting stacks
            2, # num of players
        )

        # preflop
        state.deal_hole(cards[0])
        state.deal_hole(cards[1])
        for i, bet in enumerate(all_history[0]):
            if i == 0 or i == 1:
                continue
            else:
                if bet == "f":
                    state.fold()
                elif bet == "c" or bet == "ch":
                    state.check_or_call()
                elif bet == "a":
                    state.complete_bet_or_raise_to(STARTING_STACK_SIZE * 2)
                else:
                    # multiply by 2 because preflop betting is in units of BB, and game is in units of SB
                    state.complete_bet_or_raise_to(int(bet) * 2)

        # check if all in occured
        if "a" in all_history[0] and "f" not in all_history[0]:
            # state.deal_board("".join(board[:3]))
            # state.deal_board(board[3])
            # state.deal_board(board[4])
            state.deal_board(3)
            state.deal_board(1)
            state.deal_board(1)
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            # switch SB and BB returns for preflop
            # NOTE: don't acc need to reverse the stack outputs cuz get_regret is only run when the game is finished
            return ((state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2)
        # no more betting
        # last_completed_round += 1
        if len(all_history) == 1:
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            # switch SB and BB returns for preflop
            return ((state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2)
        
        # flop
        stacks_before_round = state.stacks.copy()
        state.deal_board("".join(board[:3]))
        for bet in all_history[1]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(all_history[1])]
                money_in = stacks_before_round[GameUtil.get_player_curr(all_history[1])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(all_history[1])])
                # state.
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        # check if all in occured
        if "a" in all_history[1] and "f" not in all_history[1]:
            # state.deal_board(board[3])
            # state.deal_board(board[4])
            state.deal_board(1)
            state.deal_board(1)
        last_completed_round += 1
        # no more betting
        if len(all_history) == 2:
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            return ((state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2)
        
        # turn
        stacks_before_round = state.stacks.copy()
        state.deal_board(board[3])
        for bet in all_history[2]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(all_history[2])]
                money_in = stacks_before_round[GameUtil.get_player_curr(all_history[2])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(all_history[2])])
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        # check if all in occured
        if "a" in all_history[2] and "f" not in all_history[2]:
            # state.deal_board(board[4])
            state.deal_board(1)
        last_completed_round += 1
        # no more betting
        if len(all_history) == 3:
            # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
            return ((state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2)

        # river
        stacks_before_round = state.stacks.copy()
        state.deal_board(board[4])
        for bet in all_history[3]:
            if bet == "f":
                state.fold()
            elif bet == "c" or bet == "ch":
                state.check_or_call()
            elif bet == "a":
                curr_stack = state.stacks[GameUtil.get_player_curr(all_history[3])]
                money_in = stacks_before_round[GameUtil.get_player_curr(all_history[3])] - curr_stack
                state.complete_bet_or_raise_to(float(money_in) + state.stacks[GameUtil.get_player_curr(all_history[3])])
            else:
                raise_amt = floor(state.total_pot_amount * float(bet)) if bet == "0.3" else ceil(state.total_pot_amount * float(bet))
                state.complete_bet_or_raise_to(raise_amt)
        
        last_completed_round += 1
        # divide them by 2 because the stacks are in units of SB, and we want to return in units of BB
        return ((state.stacks[0] - (STARTING_STACK_SIZE * 2)) / 2, (state.stacks[1] - (STARTING_STACK_SIZE * 2)) / 2)

    @staticmethod
    def is_hand_done(history):
        """
        Returns the status of if the hand is done (no more actions to do).
        """
        # no betting has occured
        if len(history) == 0:
            return False
        # no betting has occured on the latest betting street
        if len(history[-1]) == 0:
            return False
        # someone folded
        if history[-1][-1] == "f":
            return True
        # someone went all in and another person called afterward
        if "a" in history[-1]:
            idx = history[-1].index("a")
            if idx != len(history[-1]) - 1:
                if history[-1][idx + 1] == "c":
                    return True
        return False

    @staticmethod
    def get_valid_actions_preflop(betting_history):
        """
        Returns all legal/possible preflop actions based on the betting history
        """
        out = GameUtil._get_possible_actions_preflop(betting_history[-1])
        return [str(a) for a in out]

    @staticmethod
    def get_valid_actions_postflop(betting_history, STARTING_STACK_SIZE=100):
        """
        Returns all legal/possible postflop actions based on the betting history
        """
        actions = GameUtil._get_possible_actions_postflop(betting_history[-1])

        # get pot size and stack sizes
        game_stats = GameUtil.get_game_stats(betting_history)
        pot_size = game_stats["pot_size"]
        stack_size = game_stats["stack_sizes"][GameUtil.get_player_curr(betting_history[-1])]

        # game stats returns net change, so add on original stack size
        stack_size += STARTING_STACK_SIZE
        # game is in units of SB
        stack_size *= 2
        pot_size *= 2
        
        # remove all raise vals that would be greater than the stack size
        out = []
        for i in range(len(actions)):
            if actions[i] == "f" or actions[i] == "c" or actions[i] == "ch" or actions[i] == "a":
                out.append(actions[i])
            elif actions[i] == "0.3" and floor(pot_size * 0.3) < stack_size:
                out.append(actions[i])
            elif actions[i] != "0.3" and ceil(pot_size * float(actions[i])) < stack_size:
                out.append(actions[i])
        
        return [str(a) for a in out]
    
    @staticmethod
    def _get_possible_actions_preflop(history, MAX_RAISES_PER_ROUND=3):
        """
        Returns all the possible actions regardless of constraints.
        """
        if history[-1] == "a":
            return ["f", "c"]
        elif len(history) == 2:
            return ["f", "c"] + GameUtil.RAISE_VALS_PREFLOP_AFTER[0] + ["a"]
        elif len(history) == 3:
            if history[2] == "c":
                return ["ch"] + GameUtil.RAISE_VALS_PREFLOP_AFTER[0] + ["a"]
            SB_raise_index = GameUtil.preflop_bet_to_index[int(history[2])]
            return ["f", "c"] + GameUtil.RAISE_VALS_PREFLOP_AFTER[SB_raise_index] + ["a"]
        elif len(history) - 2 >= MAX_RAISES_PER_ROUND:
            return ["f", "c"]
        prev_raise_index = GameUtil.preflop_bet_to_index[int(history[-1])]
        return ["f", "c"] + GameUtil.RAISE_VALS_PREFLOP_AFTER[prev_raise_index] + ["a"]

    @staticmethod
    def _get_possible_actions_postflop(history, MAX_RAISES_PER_ROUND=3):
        """
        Returns all the possible actions regardless of constraints
        """
        if len(history) == 0:
            return ["ch"] + GameSetup.get_raise_vals_postflop() + ["a"]
        if history[-1] == "ch" and len(history) == 1:
            return ["ch"] + GameSetup.get_raise_vals_postflop() + ["a"]
        
        if history[-1] == "a":
            return ["f", "c"]
        
        # check doesn't count as a raise
        if history[0] == "ch" and len(history) - 1 >= MAX_RAISES_PER_ROUND:
            return ["f", "c"]
        if history[0] != "ch" and len(history) >= MAX_RAISES_PER_ROUND:
            return ["f", "c"]
        prev_raise_index = GameUtil.postflop_bet_to_index[history[-1]]
        return ["f", "c"] + GameUtil.RAISE_VALS_POSTFLOP_AFTER[prev_raise_index] + ["a"]
    
    @staticmethod
    def copy_history(history):
        """
        Deep copies a game history
        """
        assert(len(history) != 0)
        assert(len(history) <= 4)
        return [street.copy() for street in history]
