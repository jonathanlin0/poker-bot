from typing import List
import json
import os


class GameSetup:
    def __init__(self):
        pass

    @staticmethod
    def get_raise_vals_preflop():
        """
        Get all possible raise values, in terms of BB, for preflop.
        """
        return [1, 2, 4, 8, 16]

    @staticmethod
    def get_raise_vals_postflop():
        """
        Get all possible raise values, in terms of the portion of the pot, for postflop.
        """
        return ["0.3", "0.5", "0.7", "1.0", "1.5", "2.0"]
    
    @staticmethod
    def get_suits() -> str:
        """
        Returns a string of all the suits in a deck of cards.
        """
        return "cdhs"

    @staticmethod
    def get_ranks():
        """
        Returns a string of all the ranks in a deck of cards.
        """
        return "AKQJT98765432"
    
    @staticmethod
    def get_all_cards() -> List[str]:
        """
        Get a list of all the possible cards in a standard deck.
        """
        suits = GameSetup.get_suits()
        ranks = GameSetup.get_ranks()
        return [r + s for r in ranks for s in suits]
    
    @staticmethod
    def sort_cards(cards):
        """
        sort the cards according to predefined behavior
        first sort by card rank: A, K, Q, J, T, 9, 8, 7, 6, 5, 4, 3, 2
        then sort by suit: c, d, h, s
        """
        ranks = GameSetup.get_ranks()

        sorted_cards = []
        # sort by rank
        for rank in ranks:
            curr_rank_cards = []
            for card in cards:
                if card[0] == rank:
                    curr_rank_cards.append(card)
            # sort by suit (just alphabetical)
            curr_rank_cards.sort()

            sorted_cards += curr_rank_cards

        return sorted_cards
    
    @staticmethod
    def get_all_hands():
        """
        Get all the possible hands that can be dealt preflop.
        """
        all_cards = GameSetup.get_all_cards()
        all_hands = []
        for i in range(len(all_cards)):
            for j in range(i + 1, len(all_cards)):
                all_hands.append("".join(GameSetup.sort_cards([all_cards[i], all_cards[j]])))
        return all_hands
    
    @staticmethod
    # all possible starting states of hands that SB and BB could have
    # returns a list of 2 element tuples
    def get_all_SB_BB_hands():
        # check if the output is in cache
        if os.path.exists("cache/SB_BB_hands.json"):
            f = open("cache/SB_BB_hands.json", "r")
            data = json.load(f)
            f.close()

            # deserialize the tuples
            out = []
            for state in data:
                state_list = state.split(",")
                out.append((state_list[0], state_list[1]))
            return out
        all_cards = GameSetup.get_all_cards()
        # all possible starting states of hands that SB and BB could have
        starting_states = []
        # create all possible starting situations
        # should be 2 * (52 choose 2)
        # iterate through all sets of 4 cards
        print("SB and BB starting hand states not found in cache.")
        print("Manually creating starting hand states...")
        for i in range(len(all_cards)):
            for j in range(i + 1, len(all_cards)):
                for k in range(j + 1, len(all_cards)):
                    for l in range(k + 1, len(all_cards)):
                        card_1 = all_cards[i]
                        card_2 = all_cards[j]
                        card_3 = all_cards[k]
                        card_4 = all_cards[l]
                        # given that you have 4 cards, create all starting states
                        starting_states.append( (card_1 + card_2, card_3 + card_4) )
                        starting_states.append( (card_1 + card_3, card_2 + card_4) )
                        starting_states.append( (card_1 + card_4, card_2 + card_3) )
                        starting_states.append( (card_2 + card_3, card_1 + card_4) )
                        starting_states.append( (card_2 + card_4, card_1 + card_3) )
                        starting_states.append( (card_3 + card_4, card_1 + card_2) )
        
        # sort the starting states
        temp = []
        for state in starting_states:
            # sort SB hand
            SB_hand = "".join(GameSetup.sort_cards([state[0][:2], state[0][2:]]))
            # sort BB hand
            BB_hand = "".join(GameSetup.sort_cards([state[1][:2], state[1][2:]]))
            temp.append((SB_hand, BB_hand))
        starting_states = temp

        # save the output to cache
        out = []
        for state in starting_states:
            out.append(state[0] + "," + state[1])
        # create /cache folder if it doesn't exist yet
        if not os.path.exists("cache"):
            os.makedirs("cache")
        with open("cache/SB_BB_hands.json", "w") as outfile:
            json.dump(out, outfile, indent=4)

        return starting_states