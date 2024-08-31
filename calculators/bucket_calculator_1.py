from util.game_setup import GameSetup

# bucketing alg: https://www.figma.com/file/0dvKDBffaBZe0boSyI6LoP/Bucket-Algorithm?type=whiteboard&node-id=0%3A1&t=VHeUeYTnDUxzRSCz-1

class BucketCalculator1:

    @staticmethod
    def get_board_texture(board: list[str]):
        """
        Get a representation of the board. 
        4 tuple: (is board double paired, max # of same rank, max # of same suit, for any given window of 5 consecutive ranks, max # of existing cards) = (a, b, c, d)
        """
        ranks = GameSetup.get_ranks()
        suits = GameSetup.get_suits()

        # see if board double paired
        seen_ranks = set()
        paired_ranks = set()
        for card in board:
            rank = card[0]
            if rank in seen_ranks:
                paired_ranks.add(rank)
            seen_ranks.add(rank)
        
        a = True if len(paired_ranks) >= 2 else False

        # get max num of cards of each rank
        rank_cnts = {rank: 0 for rank in ranks}
        for card in board:
            rank_cnts[card[0]] += 1
        b = max(rank_cnts.values())

        # get max num of cards of each suit
        suit_cnts = {suit: 0 for suit in suits}
        for card in board:
            suit_cnts[card[1]] += 1
        c = max(suit_cnts.values())

        # get max num of cards in any given window of 5 consecutive ranks
        existing_ranks = set()
        for card in board:
            existing_ranks.add(card[0])
        d = 0
        for i in range(len(ranks) - 4):
            curr_cnt = 0
            for j in range(i, i + 5):
                if ranks[j] in existing_ranks:
                    curr_cnt += 1
            d = max(d, curr_cnt)
        # check ace low straight
        cnt = 0
        for i in ["A", "2", "3", "4", "5"]:
            if i in existing_ranks:
                cnt += 1
        d = max(d, cnt)

        # num_to_str_category = ["low", "medium", "high"]
        # get category of highest rank
        ranks = [card[0] for card in board]
        sorted_ranks = sorted([BucketCalculator1.get_rank_category(rank) for rank in ranks])
        e = sorted_ranks[-1]

        # get median of ranks
        f = sorted_ranks[len(sorted_ranks) // 2]
        
        return (a, b, c, d, e, f)

    @staticmethod
    def get_hand_texture(hand: list[str], board: list[str], board_texture: tuple[bool, int, int, int]):
        """
        Get the hand texture. Represented as a string
        """
        existing_ranks = []
        existing_suits = []

        ranks = GameSetup.get_ranks()
        suits = GameSetup.get_suits()

        rank_cnts = {rank: 0 for rank in ranks}
        suit_cnts = {suit: 0 for suit in suits}
        for card in hand + board:
            rank = card[0]
            suit = card[1]
            rank_cnts[rank] += 1
            suit_cnts[suit] += 1
            existing_ranks.append(rank)
            existing_suits.append(suit)
        
        # check quads
        if 4 in rank_cnts.values():
            return ("quads", "")

        # check boat
        if 3 in rank_cnts.values() and 2 in rank_cnts.values():
            return ("full_house", "")

        # check flush
        if max(suit_cnts.values()) >= 5:
            return ("flush", "")
        
        # check straight
        # have to have at least 5 card window of consec ranks on board w 3 cards filled (so ur two cards can fill in the other 2 spots)
        if board_texture[3] >= 3:
            has_straight = False
            # check ace low straight
            cnt = 0
            for i in ["A", "2", "3", "4", "5"]:
                if i in existing_ranks:
                    cnt += 1
            if cnt >= 5:
                has_straight = True
            # check other straights
            for i in range(len(ranks) - 4):
                if has_straight:
                    break
                curr_cnt = 0
                for j in range(i, i + 5):
                    if ranks[j] in existing_ranks:
                        curr_cnt += 1
                if curr_cnt >= 5:
                    has_straight = True

            if has_straight:
                # check if flush draw or not
                if 4 in suit_cnts.values():
                    return ("straight", "draw")
                return ("straight", "no_draws")
        
        # check flush and straight draws. will be used on any hands WORSE than a straight
        # check flush draw
        draw = "no_draws"
        suit_cnts = {suit: 0 for suit in suits}
        for card in [hand[0], hand[1]] + board:
            suit_cnts[card[1]] += 1
        if 4 in suit_cnts.values():
            draw = "draw"
        if draw == "no_draws":
            # check straight draw
            # check ace low straight
            cnt = 0
            for i in ["A", "2", "3", "4", "5"]:
                if i in existing_ranks:
                    cnt += 1
            if cnt == 4:
                draw = "draw"
            # check other straights
            for i in range(len(ranks) - 4):
                if draw == "draw":
                    break
                curr_cnt = 0
                for j in range(i, i + 5):
                    if ranks[j] in existing_ranks:
                        curr_cnt += 1
                if curr_cnt == 4:
                    draw = "draw"
        
        # check the rest of the hands
        
        # check three of a kind (don't have to differentiate between trips and set, cuz it's implied through board texture. if paired board and u have trips, then u have a boat. if unpaired board and u have trips, then u have a set)
        if 3 in rank_cnts.values():
            return ("three_of_kind", draw)

        # check two pair
        if list(rank_cnts.values()).count(2) == 2:
            return ("two_pair", draw)
            
        # category 0: A - T. category 1: 9 - 2
        def card_category(card):
            if card[0] in ["A", "K", "Q", "J", "T"]:
                return 0
            return 1

        # check overpair
        if hand[0][0] == hand[1][0]:
            overpair = True
            for rank in existing_ranks:
                if ranks.index(rank) < ranks.index(hand[0][0]):
                    overpair = False
            if overpair:
                return ("over_pair", draw)
        
        # check top pair
        highest_rank_idx = 12
        for card in board:
            rank = card[0]
            highest_rank_idx = min(highest_rank_idx, ranks.index(rank))
        bottom_rank_idx = 1
        for card in board:
            rank = card[0]
            bottom_rank_idx = max(bottom_rank_idx, ranks.index(rank))
        if hand[0][0] == ranks[highest_rank_idx]:
            return ("top_pair", draw)
            # return ("top_pair" + str(card_category(hand[1])) + "_kicker", draw)
        if hand[1][0] == ranks[highest_rank_idx]:
            return ("top_pair", draw)
            # return ("top_pair" + str(card_category(hand[0])) + "_kicker", draw)
        
        # check middle pair
        # includes middle pocket pairs
        for card in board:
            rank = card[0]
            if (ranks.index(rank) > highest_rank_idx and ranks.index(rank) < bottom_rank_idx) == False:
                continue
            if hand[0][0] == rank:
                return ("mid_pair", draw)
                # return ("mid_pair_w/_" + str(card_category(hand[1])) + "_kicker", draw)
            if hand[1][0] == rank:
                return ("mid_pair", draw)
                # return ("mid_pair_w/_" + str(card_category(hand[0])) + "_kicker", draw)
        if hand[0][0] == hand[1][0] and ranks.index(hand[0][0]) > highest_rank_idx and ranks.index(hand[0][0]) < bottom_rank_idx:
            return ("mid_pair", draw)
            # return ("mid_pair_w/_" + str(card_category(hand[0])) + "_kicker", draw)

        # check bottom pair
        if hand[0][0] == ranks[bottom_rank_idx]:
            return ("bot_pair", draw)
            # return ("bot_pair" + str(card_category(hand[1])) + "_kicker", draw)
        if hand[1][0] == ranks[bottom_rank_idx]:
            return ("bot_pair", draw)
            # return ("bot_pair" + str(card_category(hand[0])) + "_kicker", draw)

        # check under pair
        if hand[0][0] == hand[1][0]:
            return ("under_pair", draw)
        
        # check high card
        # get best high card
        best_high_card_idx = ranks.index(hand[0][0])
        best_high_card_idx = min(best_high_card_idx, ranks.index(hand[1][0]))
        return ("high_card_cat_" + str(card_category(ranks[best_high_card_idx])), draw)
        
    @staticmethod
    def get_postflop_bucket(hand: list[str], board: list[str]):
        """
        Get the bucket for postflop.
        """
        assert(len(hand) == 2)
        assert(len(board) >= 3)
        betting_round = 1
        if len(board) == 4:
            betting_round = 2
        elif len(board) == 5:
            betting_round = 3

        board_texture = BucketCalculator1.get_board_texture(board)
        # this returns a tuple
        hand_texture = BucketCalculator1.get_hand_texture(hand, board, board_texture)
        board_texture = " ".join([str(i) for i in board_texture])
        # don't include draw if it's river betting
        # although having draws adds nuance to the hand for river, the granularity of the bucketing is too small to justify the exponetnially more training time required to reach nash
        hand_texture = hand_texture[0] if betting_round == 3 else hand_texture[0] + " " + hand_texture[1]
        
        # hand_texture = hand_texture[0] + " " + hand_texture[1]

        return board_texture + " " + hand_texture

    @staticmethod
    def get_preflop_bucket(hand):
        """
        Get the bucket for preflop. Converts a given hand (e.g. "AhKh") to a bucket (e.g. "AKs").
        """
        assert(len(hand) == 4)
        # sort the hand
        ranks = ""
        for rank in GameSetup.get_ranks():
            if hand[0] == rank:
                ranks += rank
            if hand[2] == rank:
                ranks += rank

        return ranks + "s" if hand[1] == hand[3] else ranks + "o"

    @staticmethod
    def get_raise_category_preflop(raise_val):
        """
        Categorizes the possible preflop raise values.
        """
        raise_val = str(raise_val)

        if raise_val == "ch" or raise_val == "c":
            return 0
        if raise_val == "1" or raise_val == "2" or raise_val == "4":
            return 1
        return 2 # 8 or 16
    
    @staticmethod
    def get_raise_category_postflop(raise_val):
        """
        Categorizes the possible postflop raise values
        """
        raise_val = str(raise_val)

        if raise_val == "ch" or raise_val == "c":
            return 0
        if raise_val == "0.3" or raise_val == "0.5" or raise_val == "0.7":
            return 1
        return 2 # 1.0 or 1.5 or 2.0

    @staticmethod 
    def get_raising_bucket(history):
        """
        Returns a string representation of the betting history.
        """
        betting_round = len(history) - 1

        if betting_round == 0:
            return ""

        assert(betting_round > 0) # no raising history if it's preflop

        # get greatest category of raise for preflop for both players
        SB_preflop_raise = 0
        BB_preflop_raise = 0

        max_category = 0
        for i in range(2, len(history[0]), 2): # start at 2 to skip the blinds
            category = BucketCalculator1.get_raise_category_preflop(history[0][i])
            max_category = max(category, max_category)
        SB_preflop_raise = max_category
        max_category = 0
        for i in range(3, len(history[0]), 2):
            category = BucketCalculator1.get_raise_category_preflop(history[0][i])
            max_category = max(category, max_category)
        BB_preflop_raise = max_category

        # calculate max raise category for each betting street (we'll never need to include river raises)
        highest_raises = [[0,0], [0,0]]
        for betting_street in range(1, min(3, len(history))):
            for p in range(2):
                for j in range(p, len(history[betting_street]), 2):
                    # have to offset betting street in highest_raises by -1 cuz skip preflop
                    highest_raises[betting_street - 1][p] = max(highest_raises[betting_street - 1][p], BucketCalculator1.get_raise_category_postflop(history[betting_street][j]))

        # trim appropriately to exclude current betting round and beyond
        highest_raises = highest_raises[:betting_round - 1]

        combined_raises = [0, 0]
        for street in highest_raises:
            combined_raises[0] += street[0]
            combined_raises[1] += street[1]
        
        # convert to string
        # for i in range(len(highest_raises)):
        #     highest_raises[i] = str(highest_raises[i][0]) + str(highest_raises[i][1])
        # highest_raises = "".join(highest_raises)
        
        return " ".join([str(SB_preflop_raise), str(BB_preflop_raise), "".join([str(i) for i in combined_raises])])

    @staticmethod
    def get_pot_size_category(pot_size: int):
        """
        Calculates the category of the pot size.
        """
        if pot_size <= 10.0:
            return "1"
        if pot_size <= 25.0:
            return "2"
        if pot_size <= 50.0:
            return "3"
        if pot_size <= 100.0:
            return "4"
        return "5"

    @staticmethod
    def get_rank_category(rank):
        """
        Gets the category of a rank.
        """
        rank_to_num = {
            "2": 2,
            "3": 3,
            "4": 4,
            "5": 5,
            "6": 6,
            "7": 7,
            "8": 8,
            "9": 9,
            "T": 10,
            "J": 11,
            "Q": 12,
            "K": 13,
            "A": 14
        }
        
        num_representation = rank_to_num[rank]
        if num_representation <= 7:
            return 0
        if num_representation <= 11:
            return 1
        return 2
    