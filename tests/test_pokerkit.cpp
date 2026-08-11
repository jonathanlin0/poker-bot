#include <gtest/gtest.h>
#include "pokerkit.hpp"
#include "card.hpp"
#include "action.hpp"

// ==================== Constructor Tests ====================

TEST(PokerKitTest, ConstructorInitializesStacksCorrectly) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    auto stacks = game.get_stacks();
    // After posting blinds: SB has 99, BB has 98
    EXPECT_EQ(stacks[0], 99);
    EXPECT_EQ(stacks[1], 98);
}

TEST(PokerKitTest, ConstructorWithSpecificHands) {
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'Q'), Card(/*suit=*/'C', /*rank=*/'J')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 99);
    EXPECT_EQ(stacks[1], 98);
}

// ==================== Betting Street Tests ====================

TEST(PokerKitTest, InitialBettingStreetIsPreflop) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    EXPECT_EQ(game.get_betting_street(), 0);
}

TEST(PokerKitTest, GameNotOverAtStart) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    EXPECT_FALSE(game.is_game_over());
}

// ==================== Fold Tests ====================

TEST(PokerKitTest, FoldEndsGame) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();  // SB folds
    EXPECT_TRUE(game.is_game_over());
}

TEST(PokerKitTest, FoldAfterGameOverThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();
    EXPECT_THROW(game.fold(), std::runtime_error);
}

// ==================== Check/Call Tests ====================

TEST(PokerKitTest, CallMatchesBigBlind) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.check_or_call();  // SB calls the BB
    auto stacks = game.get_stacks();
    // SB should have called to 2, so stack is now 98
    EXPECT_EQ(stacks[0], 98);
}

TEST(PokerKitTest, CallAfterGameOverThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();
    EXPECT_THROW(game.check_or_call(), std::runtime_error);
}

// ==================== Raise Tests ====================

TEST(PokerKitTest, RaiseDeductsFromStack) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.raise(8);  // SB raises by 8 (total bet = 2 + 8 = 10)
    auto stacks = game.get_stacks();
    // SB started with 99 after posting blind, raised to 10 (added 9 more)
    EXPECT_EQ(stacks[0], 90);
}

TEST(PokerKitTest, RaiseMoreThanStackThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    // SB has 99 chips after posting 1 blind, cannot raise to 200
    EXPECT_THROW(game.raise(200), std::runtime_error);
}

TEST(PokerKitTest, RaiseAfterGameOverThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();
    EXPECT_THROW(game.raise(10), std::runtime_error);
}

// ==================== Raise Legality Tests ====================

TEST(PokerKitTest, RaiseLessThanMinimumThrows_Preflop) {
    // Preflop: min raise = max(BB - SB, BB) = max(1, 2) = 2
    // Raising by 1 should throw (less than BB)
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    EXPECT_THROW(game.raise(1), std::runtime_error);
}

TEST(PokerKitTest, RaiseExactlyMinimumSucceeds_Preflop) {
    // Preflop: min raise = max(BB - SB, BB) = max(1, 2) = 2
    // Raising by exactly 2 should succeed (total bet = 2 + 2 = 4)
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    EXPECT_NO_THROW(game.raise(2));
    auto stacks = game.get_stacks();
    // SB had 99, raised to 4 (added 3 more chips)
    EXPECT_EQ(stacks[0], 96);
}

TEST(PokerKitTest, ReRaiseLessThanPreviousRaiseThrows) {
    // SB raises by 8 (total = 10), previous raise increment = 10 - 2 = 8
    // BB tries to re-raise by 5, which is less than 8 → should throw
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.raise(8);  // SB raises by 8 (total = 10)
    EXPECT_THROW(game.raise(5), std::runtime_error);  // BB tries to raise by 5
}

TEST(PokerKitTest, ReRaiseExactlyPreviousRaiseSucceeds) {
    // SB raises by 8 (total = 10), previous raise increment = 10 - 2 = 8
    // BB re-raises by exactly 8 (total = 10 + 8 = 18) → should succeed
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.raise(8);  // SB raises by 8 (total = 10)
    EXPECT_NO_THROW(game.raise(8));  // BB re-raises by 8 (total = 18)
    auto stacks = game.get_stacks();
    // SB: 100 - 10 = 90
    // BB: 100 - 18 = 82
    EXPECT_EQ(stacks[0], 90);
    EXPECT_EQ(stacks[1], 82);
}

TEST(PokerKitTest, ReRaiseMoreThanPreviousRaiseSucceeds) {
    // SB raises by 8 (total = 10), previous raise increment = 8
    // BB re-raises by 20 (total = 10 + 20 = 30) → should succeed
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.raise(8);  // SB raises by 8 (total = 10)
    EXPECT_NO_THROW(game.raise(20));  // BB re-raises by 20 (total = 30)
    auto stacks = game.get_stacks();
    // SB: 100 - 10 = 90
    // BB: 100 - 30 = 70
    EXPECT_EQ(stacks[0], 90);
    EXPECT_EQ(stacks[1], 70);
}

// ==================== Card Tests ====================

TEST(CardTest, ValidCardCreation) {
    EXPECT_NO_THROW(Card(/*suit=*/'H', /*rank=*/'A'));
    EXPECT_NO_THROW(Card(/*suit=*/'S', /*rank=*/'K'));
    EXPECT_NO_THROW(Card(/*suit=*/'D', /*rank=*/'T'));
    EXPECT_NO_THROW(Card(/*suit=*/'C', /*rank=*/'2'));
}

TEST(CardTest, InvalidSuitThrows) {
    EXPECT_THROW(Card('X', 'A'), std::runtime_error);
}

TEST(CardTest, InvalidRankThrows) {
    EXPECT_THROW(Card('H', 'X'), std::runtime_error);
}

TEST(CardTest, CardComparisonOperators) {
    Card ace('H', 'A');
    Card king('S', 'K');
    Card queen('D', 'Q');
    Card ace2('C', 'A');
    
    EXPECT_TRUE(ace > king);
    EXPECT_TRUE(king > queen);
    EXPECT_TRUE(queen < ace);
    EXPECT_TRUE(ace == ace2);  // Same rank, different suit
    EXPECT_FALSE(ace != ace2);
}

// ==================== Action Tests ====================

TEST(ActionTest, ActionCreation) {
    Action call(/*type=*/'c', /*amount=*/10);
    EXPECT_EQ(call.type, 'c');
    EXPECT_EQ(call.amount, 10);
    
    Action fold(/*type=*/'f', /*amount=*/0);
    EXPECT_EQ(fold.type, 'f');
    EXPECT_EQ(fold.amount, 0);
    
    Action raise(/*type=*/'r', /*amount=*/50);
    EXPECT_EQ(raise.type, 'r');
    EXPECT_EQ(raise.amount, 50);
}

// ==================== Integration Tests (Simple Game Flows) ====================

TEST(PokerKitIntegrationTest, PreflopFoldBySB) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();  // SB folds preflop
    EXPECT_TRUE(game.is_game_over());
    
    // SB posted 1 and folded → loses 1 chip → stack = 99
    // BB posted 2 and wins the pot (1 + 2 = 3) → stack = 100 - 2 + 3 = 101
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 99);   // SB
    EXPECT_EQ(stacks[1], 101);  // BB
}

TEST(PokerKitIntegrationTest, PreflopCallThenCheck) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.check_or_call();  // SB calls
    game.check_or_call();  // BB checks
    // Still on preflop until board is dealt
    EXPECT_EQ(game.get_betting_street(), 0);
    
    // Deal flop (3 cards) to advance to street 1
    game.deal_board();
    EXPECT_EQ(game.get_betting_street(), 0);
    game.deal_board();
    EXPECT_EQ(game.get_betting_street(), 0);
    game.deal_board();
    EXPECT_EQ(game.get_betting_street(), 1);
}

// ==================== Deal Board Tests ====================

TEST(PokerKitTest, DealBoardWithSpecificCard) {
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'Q'), Card(/*suit=*/'C', /*rank=*/'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    game.check_or_call();  // SB calls
    game.check_or_call();  // BB checks
    
    // Deal specific flop cards
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'3'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'4'));
    
    EXPECT_EQ(game.get_betting_street(), 1);
}

TEST(PokerKitTest, DealBoardDuplicateCardThrows) {
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'Q'), Card(/*suit=*/'C', /*rank=*/'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    game.check_or_call();
    game.check_or_call();
    
    // Try to deal a card that's already in someone's hand
    EXPECT_THROW(game.deal_board(Card(/*suit=*/'H', /*rank=*/'A')), std::runtime_error);
}

TEST(PokerKitTest, DealBoardCardAlreadyOnBoardThrows) {
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'Q'), Card(/*suit=*/'C', /*rank=*/'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    // Try to deal the same card again
    EXPECT_THROW(game.deal_board(Card(/*suit=*/'H', /*rank=*/'2')), std::runtime_error);
}

TEST(PokerKitTest, DealBoardCardInBBHandThrows) {
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'Q'), Card(/*suit=*/'C', /*rank=*/'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    game.check_or_call();
    game.check_or_call();
    
    // Try to deal a card that's in BB's hand
    EXPECT_THROW(game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q')), std::runtime_error);
}

TEST(PokerKitTest, DealHandsWithDuplicateCardThrows) {
    // Both players dealt the same card
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'J')};  // Ah already in SB's hand
    
    EXPECT_THROW(
        PokerKit(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand),
        std::runtime_error
    );
}

TEST(PokerKitTest, DealBoardAfterGameOverThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();  // Game ends
    EXPECT_THROW(game.deal_board(), std::runtime_error);
}

TEST(PokerKitTest, DealMoreThan5BoardCardsThrows) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.check_or_call();
    game.check_or_call();
    
    // Deal flop
    game.deal_board();
    game.deal_board();
    game.deal_board();
    
    // Deal turn
    game.deal_board();
    
    // Deal river
    game.deal_board();
    
    // 6th card should throw
    EXPECT_THROW(game.deal_board(), std::runtime_error);
}

// ==================== All-In Tests ====================

TEST(PokerKitTest, AllIn_SetsStackToZero) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.all_in();  // SB goes all-in
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 98);  // BB hasn't acted yet, still has 100 - 2 (blind)
}

TEST(PokerKitTest, AllIn_RecordsCorrectAction) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();  // SB all-in: effective = 99 + 1 (blind) = 100
    auto bets = game.get_bets();
    auto& preflop = bets[0];
    EXPECT_EQ(preflop.size(), 3);  // blind(1), blind(2), all_in(100)
    EXPECT_EQ(preflop[2].type, 'a');
    EXPECT_EQ(preflop[2].amount, 100);
}

TEST(PokerKitTest, AllIn_PotUpdatesCorrectly) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.all_in();  // SB all-in
    // pot was 3 (1 SB + 2 BB), SB adds 99 more = 102
    EXPECT_EQ(game.get_pot_size(), 102);
}

TEST(PokerKitTest, AllIn_AfterRaise) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.raise(8);  // SB raises by 8 (total bet = 2 + 8 = 10)
    game.all_in();   // BB all-in: effective = 98 + 2 (blind) = 100
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 90);  // SB raised to 10, stack = 100 - 10 = 90
    EXPECT_EQ(stacks[1], 0);
    auto bets = game.get_bets();
    EXPECT_EQ(bets[0][3].type, 'a');
    EXPECT_EQ(bets[0][3].amount, 100);
}

TEST(PokerKitTest, AllIn_AfterRaiseAndReraise) {
    // SB raises to 25, BB reraises to 75, SB goes all-in
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.raise(23);   // SB raises by 23 (total = 2 + 23 = 25), stack = 75
    game.raise(50);   // BB reraises by 50 (total = 25 + 50 = 75), stack = 25
    game.all_in();    // SB all-in: effective = 75 + 25 = 100
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 25);  // BB has 100 - 75 = 25 remaining
    auto bets = game.get_bets();
    EXPECT_EQ(bets[0][4].type, 'a');
    EXPECT_EQ(bets[0][4].amount, 100);
    // pot: 3 (blinds) + 24 (SB raise) + 73 (BB reraise) + 75 (SB all-in) = 175
    EXPECT_EQ(game.get_pot_size(), 175);
}

TEST(PokerKitTest, AllIn_ThrowsWhenGameOver) {
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.fold();
    EXPECT_THROW(game.all_in(), std::runtime_error);
}

// ==================== Can Still Bet Tests ====================

TEST(PokerKitTest, CanStillCallAfterAllIn) {
    // After one player goes all-in, the other should still be able to call
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.all_in();  // SB goes all-in
    EXPECT_TRUE(game.get_can_still_bet());  // BB hasn't called yet
    EXPECT_FALSE(game.is_game_over());
    EXPECT_NO_THROW(game.check_or_call());  // BB calls
    EXPECT_FALSE(game.get_can_still_bet());
}

TEST(PokerKitTest, CanStillFoldAfterAllIn) {
    // After one player goes all-in, the other can still fold
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.all_in();  // SB goes all-in
    EXPECT_TRUE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    EXPECT_NO_THROW(game.fold());  // BB folds
}

TEST(PokerKitTest, CantCheckOrCallAfterAllInAndCall) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();        // SB all-in
    game.check_or_call(); // BB calls → can_still_bet = false
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());  // Still need to deal board cards
    EXPECT_THROW(game.check_or_call(), std::runtime_error);
}

TEST(PokerKitTest, CantFoldAfterAllInAndCall) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();
    game.check_or_call();
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    EXPECT_THROW(game.fold(), std::runtime_error);
}

TEST(PokerKitTest, CantRaiseAfterAllInAndCall) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();
    game.check_or_call();
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    EXPECT_THROW(game.raise(10), std::runtime_error);
}

TEST(PokerKitTest, CantAllInAfterAllInAndCall) {
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();
    game.check_or_call();
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    EXPECT_THROW(game.all_in(), std::runtime_error);
}

TEST(PokerKitTest, CantRaiseOrAllInAfterOpponentAllIn) {
    // After one player goes all-in, the other can only call or fold (not raise or all-in)
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100);
    game.all_in();  // SB goes all-in
    EXPECT_THROW(game.raise(50), std::runtime_error);
    EXPECT_THROW(game.all_in(), std::runtime_error);
    EXPECT_NO_THROW(game.check_or_call());  // Call is valid
}

TEST(PokerKitTest, DealBoardStillWorksAfterAllInAndCall) {
    // After all-in + call preflop, deal_board should still work (manual dealing)
    // P0 wins with AK high vs QJ high on board K Q T 7 2
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();
    game.check_or_call();
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 0);
    EXPECT_EQ(game.get_pot_size(), 200);
    // Can still deal all 5 board cards, game ends on river
    game.deal_board(Card('D', 'K'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('C', 'Q'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('S', 'T'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('H', '7'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('H', '2'));
    // River dealt → game over, pot settled
    EXPECT_TRUE(game.is_game_over());
    stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 200);  // P0 wins with A-high (AK > QJ)
    EXPECT_EQ(stacks[1], 0);
}

TEST(PokerKitTest, AllIn_OnFlopCallThenDealBoard) {
    // All-in on the flop, then deal remaining board cards
    // P0 wins with pair of Aces vs QJ on board K 9 7 5 2 (no straight/flush)
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'A')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop: call/check
    game.check_or_call();
    game.check_or_call();
    EXPECT_TRUE(game.get_can_still_bet());
    
    // Deal flop
    game.deal_board(Card('D', 'K'));
    game.deal_board(Card('S', '9'));
    game.deal_board(Card('C', '7'));
    
    // Flop: SB goes all-in, BB calls
    game.all_in();
    EXPECT_TRUE(game.get_can_still_bet());  // BB hasn't called yet
    EXPECT_FALSE(game.is_game_over());
    game.check_or_call();
    
    // Betting is locked, but game is NOT over (still need turn + river)
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 0);
    EXPECT_EQ(game.get_pot_size(), 200);
    
    EXPECT_THROW(game.check_or_call(), std::runtime_error);
    EXPECT_THROW(game.raise(10), std::runtime_error);
    
    // Deal turn — game still not over
    EXPECT_NO_THROW(game.deal_board(Card('H', '5')));
    EXPECT_FALSE(game.is_game_over());
    
    // Deal river — game ends, pot settled
    EXPECT_NO_THROW(game.deal_board(Card('D', '2')));
    EXPECT_TRUE(game.is_game_over());
    stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 200);  // P0 wins with pair of Aces
    EXPECT_EQ(stacks[1], 0);
}

TEST(PokerKitTest, AllIn_OnTurnCallThenDealRiver) {
    // All-in on the turn, deal river, game ends
    // P1 wins with pair of Aces vs 23 on board K Q T 7 5
    std::array<Card, 2> sb_hand = {Card('C', '2'), Card('D', '3')};
    std::array<Card, 2> bb_hand = {Card('H', 'A'), Card('S', 'A')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Flop
    game.deal_board(Card('D', 'K'));
    game.deal_board(Card('C', 'Q'));
    game.deal_board(Card('S', 'T'));
    game.check_or_call();
    game.check_or_call();
    
    // Turn: SB checks, BB all-in, SB calls
    game.deal_board(Card('H', '7'));
    game.check_or_call();  // SB checks
    game.all_in();          // BB all-in
    EXPECT_TRUE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    game.check_or_call();  // SB calls
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());  // Still need river
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 0);
    EXPECT_EQ(game.get_pot_size(), 200);
    
    // Deal river → game ends
    game.deal_board(Card('D', '5'));
    EXPECT_TRUE(game.is_game_over());
    stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 200);  // P1 wins with pair of Aces
}

TEST(PokerKitTest, AllIn_StacksCorrectAfterCallPreflop) {
    // Both stacks should be 0 after preflop all-in + call
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();        // SB all-in for 100
    game.check_or_call(); // BB calls 100
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());  // Still need to deal board
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 0);
    EXPECT_EQ(game.get_pot_size(), 200);
}

// ==================== All-In Settlement Tests ====================

TEST(PokerKitTest, AllIn_PreflopFoldSettlesPot) {
    // SB goes all-in, BB folds → SB wins the pot
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'K')};
    std::array<Card, 2> bb_hand = {Card('D', 'Q'), Card('C', 'J')};
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    game.all_in();  // SB all-in for 100, pot = 102
    game.fold();    // BB folds
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // SB wins pot
    EXPECT_EQ(stacks[1], 98);   // BB loses 2 (blind)
}

TEST(PokerKitTest, AllIn_OnRiverSettlesPot_Player0Wins) {
    // Full game to river, SB goes all-in, BB calls. P0 wins with pair of Aces.
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'A')};
    std::array<Card, 2> bb_hand = {Card('C', '2'), Card('D', '3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop: SB calls, BB checks
    game.check_or_call();
    game.check_or_call();
    
    // Flop
    game.deal_board(Card('D', 'K'));
    game.deal_board(Card('C', 'Q'));
    game.deal_board(Card('S', 'T'));
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card('H', '7'));
    game.check_or_call();
    game.check_or_call();
    
    // River: SB goes all-in, BB calls
    game.deal_board(Card('D', '5'));
    game.all_in();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 200);  // P0 wins everything with pair of Aces
    EXPECT_EQ(stacks[1], 0);
}

TEST(PokerKitTest, AllIn_OnRiverSettlesPot_Player1Wins) {
    // Full game to river, SB checks, BB goes all-in, SB calls. P1 wins with pair of Aces.
    std::array<Card, 2> sb_hand = {Card('C', '2'), Card('D', '3')};
    std::array<Card, 2> bb_hand = {Card('H', 'A'), Card('S', 'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Flop
    game.deal_board(Card('D', 'K'));
    game.deal_board(Card('C', 'Q'));
    game.deal_board(Card('S', 'T'));
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card('H', '7'));
    game.check_or_call();
    game.check_or_call();
    
    // River: SB checks, BB goes all-in, SB calls
    game.deal_board(Card('D', '5'));
    game.check_or_call();  // SB checks
    game.all_in();          // BB all-in
    game.check_or_call();  // SB calls
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 200);  // P1 wins everything with pair of Aces
}

TEST(PokerKitTest, AllIn_AfterRaiseAndReraiseSettlesPot) {
    // SB raises to 25, BB reraises to 75, SB all-in, BB calls → full pot = 200
    // P0 wins with pair of Aces
    std::array<Card, 2> sb_hand = {Card('H', 'A'), Card('S', 'A')};
    std::array<Card, 2> bb_hand = {Card('C', '2'), Card('D', '3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop: raise war → all-in
    game.raise(23);        // SB raises by 23 (total = 2 + 23 = 25)
    game.raise(50);        // BB reraises by 50 (total = 25 + 50 = 75)
    game.all_in();         // SB all-in for 100
    game.check_or_call();  // BB calls 100
    
    // Both stacks 0, pot 200, can't bet, game not over yet
    EXPECT_FALSE(game.get_can_still_bet());
    EXPECT_FALSE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 0);
    EXPECT_EQ(stacks[1], 0);
    EXPECT_EQ(game.get_pot_size(), 200);
    
    // Deal full board — game ends on river
    game.deal_board(Card('D', 'K'));
    game.deal_board(Card('C', 'Q'));
    game.deal_board(Card('S', 'T'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('H', '7'));
    EXPECT_FALSE(game.is_game_over());
    game.deal_board(Card('D', '5'));
    EXPECT_TRUE(game.is_game_over());
    stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 200);  // P0 wins with pair of Aces
    EXPECT_EQ(stacks[1], 0);
}

// ==================== Hand Evaluation Tests ====================

// ==================== Straight Flush Tests ====================

TEST(HandEvaluationTest, StraightFlush_Player0Wins) {
    // P0 (SB) has royal flush in hearts
    // P1 (BB) has nothing special
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop: SB calls, BB checks
    game.check_or_call();  // SB calls to 2
    game.check_or_call();  // BB checks
    
    // Deal flop: Qh, Jh, Th (completes P0's royal flush)
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    
    // Flop: BB checks, SB checks
    game.check_or_call();
    game.check_or_call();
    
    // Deal turn
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    
    // Turn: BB checks, SB checks
    game.check_or_call();
    game.check_or_call();
    
    // Deal river
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'6'));
    
    // River: BB checks, SB checks
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 (SB) wins the pot
    EXPECT_EQ(stacks[1], 98);   // P1 (BB) loses
}

TEST(HandEvaluationTest, StraightFlush_Player1Wins) {
    // P0 (SB) has nothing special
    // P1 (BB) has royal flush in hearts
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();  // SB calls
    game.check_or_call();  // BB checks
    
    // Deal flop: Qh, Jh, Th (completes P1's royal flush)
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    
    // Flop
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    // River
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'6'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);   // P0 (SB) loses
    EXPECT_EQ(stacks[1], 102);  // P1 (BB) wins the pot
}

TEST(HandEvaluationTest, StraightFlush_BothHave_Player0Higher) {
    // P0 (SB) has A-high straight flush (royal flush)
    // P1 (BB) has Q-high straight flush
    // Board: Qh, Jh, Th
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};  // A-K-Q-J-T
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'9'), Card(/*suit=*/'H', /*rank=*/'8')};  // Q-J-T-9-8
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Deal flop
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    
    // Flop
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    // River
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with higher straight flush
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, StraightFlush_BothHave_Player1Higher) {
    // P0 (SB) has Q-high straight flush
    // P1 (BB) has A-high straight flush (royal flush)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'9'), Card(/*suit=*/'H', /*rank=*/'8')};  // Q-J-T-9-8
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};  // A-K-Q-J-T
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Deal flop
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    
    // Flop
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    // River
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with higher straight flush
}

TEST(HandEvaluationTest, StraightFlush_BothHave_Chop) {
    // Both players use the board's straight flush (royal flush on board)
    // Neither player has hearts in their hand
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'S', /*rank=*/'4'), Card(/*suit=*/'C', /*rank=*/'5')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Deal flop: Royal flush on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    // Flop
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.check_or_call();
    game.check_or_call();
    
    // River
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: pot split evenly, both get their money back
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, StraightFlush_Wheel_Player0Wins) {
    // P0 (SB) has wheel straight flush (A-2-3-4-5 of hearts)
    // P1 (BB) has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'2')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'8')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Deal flop: 3h, 4h, 5h (completes wheel straight flush)
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'4'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'5'));
    
    // Flop
    game.check_or_call();
    game.check_or_call();
    
    // Turn
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    // River
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with wheel straight flush
    EXPECT_EQ(stacks[1], 98);
}

// ==================== Four of a Kind (Quads) Tests ====================

TEST(HandEvaluationTest, Quads_Player0Wins) {
    // P0 (SB) has quad Aces
    // P1 (BB) has nothing special
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Ac, 5s, 7h, 9d → P0 has quad Aces
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with quad Aces
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Quads_Player1Wins) {
    // P0 (SB) has nothing special
    // P1 (BB) has quad Aces
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Ac, 5s, 7h, 9d → P1 has quad Aces
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with quad Aces
}

TEST(HandEvaluationTest, Quads_BothHave_Player0Higher) {
    // P0 (SB) has quad Kings, P1 (BB) has quad Queens
    // Board has 2 Kings and 2 Queens
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Kc, Qd, Qc, 5s
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with quad Kings > quad Queens
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Quads_BothHave_Player1Higher) {
    // P0 (SB) has quad Queens, P1 (BB) has quad Kings
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Kc, Qd, Qc, 5s
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with quad Kings > quad Queens
}

TEST(HandEvaluationTest, Quads_BothHave_Chop) {
    // Quad Aces on the board, neither player improves the kicker
    // Board kicker (K) plays for both
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'4'), Card(/*suit=*/'S', /*rank=*/'5')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, As, Ad, Ac, Ks → quad Aces on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: both play board's quad Aces + King kicker
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, Quads_SameQuads_Player0BetterKicker) {
    // Board has quad Aces, P0 has King kicker, P1 has Queen kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'D', /*rank=*/'2')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, As, Ad, Ac, 5s → quad Aces on board, 5 kicker on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // P0 wins: quad Aces + K kicker > quad Aces + Q kicker
    EXPECT_EQ(stacks[0], 102);
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Quads_SameQuads_Player1BetterKicker) {
    // Board has quad Aces, P0 has Queen kicker, P1 has King kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'Q'), Card(/*suit=*/'D', /*rank=*/'2')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, As, Ad, Ac, 5s → quad Aces on board, 5 kicker on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // P1 wins: quad Aces + K kicker > quad Aces + Q kicker
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);
}

// ==================== Full House Tests ====================

TEST(HandEvaluationTest, FullHouse_Player0Wins) {
    // P0 (SB) has Aces full of Kings (AAA-KK)
    // P1 (BB) has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'D', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kh, Kd, 7s, 9c → P0 has AAA-KK
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with full house
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, FullHouse_Player1Wins) {
    // P0 (SB) has nothing
    // P1 (BB) has Aces full of Kings (AAA-KK)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'D', /*rank=*/'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kh, Kd, 7s, 9c → P1 has AAA-KK
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with full house
}

TEST(HandEvaluationTest, FullHouse_BothHave_Player0Higher) {
    // Both have pocket pairs, board has trip Aces
    // P0 (SB) has KK → AAA-KK (Aces full of Kings)
    // P1 (BB) has QQ → AAA-QQ (Aces full of Queens)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ac, 5s, 7d → both have full houses
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AAA-KK > AAA-QQ
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, FullHouse_BothHave_Player1Higher) {
    // Both have pocket pairs, board has trip Aces
    // P0 (SB) has QQ → AAA-QQ
    // P1 (BB) has KK → AAA-KK
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ac, 5s, 7d
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AAA-KK > AAA-QQ
}

TEST(HandEvaluationTest, FullHouse_BothHave_HigherTrips_Player0Wins) {
    // Both have full houses with different trips
    // P0 has KKK-AA (trip Kings > trip Queens)
    // P1 has QQQ-AA
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qd, Ac, As, 7d
    // P0: Kh+Ks+Kd = trip Kings, Ac+As = pair Aces → KKK-AA
    // P1: Qh+Qs+Qd = trip Queens, Ac+As = pair Aces → QQQ-AA
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: KKK-AA > QQQ-AA (trips compared first)
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, FullHouse_BothHave_HigherTrips_Player1Wins) {
    // P0 has QQQ-AA, P1 has KKK-AA
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qd, Ac, As, 7d
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: KKK-AA > QQQ-AA (trips compared first)
}

TEST(HandEvaluationTest, FullHouse_BothHave_Chop) {
    // Both have same card (different suit) to make identical full house
    // P0 has Kh, P1 has Kd → both make AAA-KK with board's Ks
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'2')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ac, Ks, 7d → both make AAA-KK
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: both have AAA-KK
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

// ==================== Flush Tests ====================

TEST(HandEvaluationTest, Flush_Player0Wins) {
    // P0 (SB) has a heart flush, P1 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 9h, 7h, 4h, 5d, 6c → P0 has A-K-9-7-4 flush in hearts
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'4'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'6'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with flush
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Flush_Player1Wins) {
    // P0 has nothing, P1 (BB) has a heart flush
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 9h, 7h, 4h, 5d, 6c → P1 has A-K-9-7-4 flush in hearts
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'4'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'6'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with flush
}

TEST(HandEvaluationTest, Flush_BothHave_Player0HigherTopCard) {
    // Both have heart flushes, P0 has higher top card
    // Board has 3 hearts, each player contributes 2 hearts
    // P0: Ah, Kh → A-high flush
    // P1: Qh, Jh → Q-high flush
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'H', /*rank=*/'J')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 9h, 7h, 4h, 5d, 6c → 3 hearts on board
    // P0: A-K-9-7-4 hearts → A-high flush
    // P1: Q-J-9-7-4 hearts → Q-high flush
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'4'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'6'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A-high flush > Q-high flush
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Flush_BothHave_Player1HigherTopCard) {
    // P0: Qh, Jh → Q-high flush
    // P1: Ah, Kh → A-high flush
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'H', /*rank=*/'J')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 9h, 7h, 4h, 5d, 6c
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'4'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'6'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A-high flush > Q-high flush
}

TEST(HandEvaluationTest, Flush_BothHave_Player0HigherFifthCard) {
    // The first four flush cards are shared; P0 wins with the fifth card.
    // P0: 7h, 3d -> A-K-Q-9-7 flush
    // P1: 6h, 4s -> A-K-Q-9-6 flush
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'6'), Card(/*suit=*/'S', /*rank=*/'4')};

    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);

    // Preflop
    game.check_or_call();
    game.check_or_call();

    // Board: Ah, Kh, Qh, 9h, 2c
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));

    game.check_or_call();
    game.check_or_call();

    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();

    game.deal_board(Card(/*suit=*/'C', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();

    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins on the fifth flush card: 7 > 6
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Flush_BothHave_SameTopFiveWithDifferentLengths_Chop) {
    // Both players use the five board hearts. Extra lower hearts do not play.
    // P0 has seven hearts total; P1 has six hearts total.
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'6'), Card(/*suit=*/'H', /*rank=*/'5')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'4'), Card(/*suit=*/'C', /*rank=*/'2')};

    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);

    // Preflop
    game.check_or_call();
    game.check_or_call();

    // Board: Ah, Kh, Qh, 9h, 7h
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));

    game.check_or_call();
    game.check_or_call();

    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();

    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();

    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, Flush_BothHave_Chop) {
    // Board has 5 hearts → both players play the board flush
    // Neither player holds a heart higher than the board
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'S', /*rank=*/'4'), Card(/*suit=*/'C', /*rank=*/'5')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kh, Qh, Jh, 9h → A-high flush on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: both play the board's A-K-Q-J-9 heart flush
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

// ==================== Straight Tests ====================

TEST(HandEvaluationTest, Straight_Player0Wins) {
    // P0 has a straight (T-high), P1 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'T'), Card(/*suit=*/'S', /*rank=*/'9')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 8d, 7c, 6s, Kh, 2h → P0 has T-9-8-7-6 straight
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'8'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'6'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with straight
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Straight_Player1Wins) {
    // P1 has a straight (T-high), P0 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'T'), Card(/*suit=*/'S', /*rank=*/'9')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 8d, 7c, 6s, Kh, Ah → P1 has T-9-8-7-6 straight
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'8'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'6'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with straight
}

TEST(HandEvaluationTest, Straight_BothHave_Player0Higher) {
    // Both have straights, P0's is higher
    // P0: Ah, Kd → A-K-Q-J-T (broadway)
    // P1: 9s, 8c → Q-J-T-9-8
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'D', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'S', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'8')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Qs, Jc, Td, 2h, 3h → P0 has A-high straight, P1 has Q-high straight
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A-high straight > Q-high straight
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Straight_BothHave_Player1Higher) {
    // P0: 9s, 8c → Q-J-T-9-8
    // P1: Ah, Kd → A-K-Q-J-T (broadway)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'S', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'8')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'D', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Qs, Jc, Td, 2h, 3h
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A-high straight > Q-high straight
}

TEST(HandEvaluationTest, Straight_BothHave_Chop) {
    // Board has a straight, neither player extends it
    // Both play the board's straight
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ks, Qd, Jc, Th → broadway on board
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'J'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'T'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: both play board's A-K-Q-J-T straight
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, Straight_Wheel_Player0Wins) {
    // P0 has wheel straight (A-2-3-4-5), P1 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'2')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'9')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: 3d, 4c, 5s, Kh, Qd → P0 has A-2-3-4-5 wheel
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'4'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with wheel straight
    EXPECT_EQ(stacks[1], 98);
}

// ==================== Three of a Kind – Set Tests (pocket pair + one on board) ====================

TEST(HandEvaluationTest, Set_Player0Wins) {
    // P0 has pocket Aces, board has an Ace → set of Aces
    // P1 has random cards, no three of a kind
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'8')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, 5c, 3s, Kh, 2d → P0 has set of Aces
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'3'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with set of Aces
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Set_Player1Wins) {
    // P1 has pocket Kings, board has a King → set of Kings
    // P0 has random cards
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'8')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, 5c, 3s, 2h, 9d → P1 has set of Kings
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'3'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with set of Kings
}

TEST(HandEvaluationTest, Set_BothHave_Player0Higher) {
    // P0 has pocket Aces, P1 has pocket Kings
    // Board has one Ace and one King → P0 set of Aces > P1 set of Kings
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Kd, 5c, 3s, 2h → P0 set of Aces, P1 set of Kings
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: set of Aces > set of Kings
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Set_BothHave_Player1Higher) {
    // P0 has pocket Kings, P1 has pocket Aces
    // Board has one Ace and one King → P1 set of Aces > P0 set of Kings
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Kd, 5c, 3s, 2h
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: set of Aces > set of Kings
}

// ==================== Three of a Kind – Trips Tests (one in hand + pair on board) ====================

TEST(HandEvaluationTest, Trips_Player0Wins) {
    // P0 has an Ace, board has two Aces → trips Aces
    // P1 has no matching card
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'8')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, As, 5c, 3h, 2d → P0 has trip Aces
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with trip Aces
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Trips_Player1Wins) {
    // P1 has a King, board has two Kings → trips Kings
    // P0 has no matching card
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'7'), Card(/*suit=*/'D', /*rank=*/'8')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Ks, 5c, 3h, 2d → P1 has trip Kings
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with trip Kings
}

TEST(HandEvaluationTest, Trips_BothHave_Player0HigherKicker) {
    // Board has pair of Aces, both players have an Ace → both have trip Aces
    // P0 has Kh kicker, P1 has Qh kicker → P0 wins on kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, As, 5c, 3h, 2d → both have trip Aces
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: trip Aces with K kicker > Q kicker
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Trips_BothHave_Player1HigherKicker) {
    // Board has pair of Aces, both players have an Ace → both have trip Aces
    // P0 has Qh kicker, P1 has Kh kicker → P1 wins on kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, As, 5c, 3h, 2d
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: trip Aces with K kicker > Q kicker
}

TEST(HandEvaluationTest, Trips_BothHave_Chop) {
    // Board has three of a kind (trip Aces), neither player has an Ace
    // Both players play the board trips + best kickers from the board
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, As, Ah, Kc, Qd → trip Aces on board, K-Q kickers on board
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    // Chop: both play board's AAA-K-Q
    EXPECT_EQ(stacks[0], 100);
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, Set_BothHave_HigherTripsRank_Player0Wins) {
    // Both have sets but of different ranks
    // P0 has pocket Aces → set of Aces
    // P1 has pocket Kings → set of Kings
    // Board has one Ace and one King
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Kd, 5c, 3h, 2s → P0 set of Aces > P1 set of Kings
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: set of Aces > set of Kings
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, ThreeOfAKind_HigherRankWins_SetVsTrips) {
    // P0 has a set (pocket Jacks + J on board)
    // P1 has trips (Th + pair of Tens on board)
    // Both have three of a kind; P0's Jacks > P1's Tens
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'J'), Card(/*suit=*/'S', /*rank=*/'J')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'T'), Card(/*suit=*/'C', /*rank=*/'2')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Jd, Td, Ts, 5c, 3h → P0 set of Jacks, P1 trip Tens
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'J'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: set of Jacks > trip Tens
    EXPECT_EQ(stacks[1], 98);
}

// ==================== Two Pair Tests ====================

TEST(HandEvaluationTest, TwoPair_Player0Wins) {
    // P0 (SB) has two pair (Aces and Kings), P1 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kd, 7c, 9h, 5d → P0 has two pair (AA + KK)
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with two pair AA + KK
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, TwoPair_Player1Wins) {
    // P0 (SB) has nothing, P1 (BB) has two pair (Aces and Kings)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kd, 7c, 9h, 5d → P1 has two pair (AA + KK)
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'9'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with two pair AA + KK
}

TEST(HandEvaluationTest, TwoPair_BothHave_Player0HigherTopPair) {
    // P0 has AA + 77, P1 has KK + 77 (board has a 7 pair, each player pairs their high card)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'9')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'9')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, 7h, 7d, 2s → P0 has AA + 77, P1 has KK + 77
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA + 77 > KK + 77
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, TwoPair_BothHave_Player1HigherTopPair) {
    // P0 has KK + 77, P1 has AA + 77
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'9')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'9')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, 7h, 7d, 2s → P0 has KK + 77, P1 has AA + 77
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA + 77 > KK + 77
}

TEST(HandEvaluationTest, TwoPair_BothHave_SameTopPair_Player0HigherSecondPair) {
    // Both pair Aces on board, P0 has KK as second pair, P1 has QQ
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kd, Qh, 7c, 2d → P0 has AA + KK, P1 has AA + QQ
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA + KK > AA + QQ
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, TwoPair_BothHave_SameTopPair_Player1HigherSecondPair) {
    // Both pair Aces on board, P0 has QQ as second pair, P1 has KK
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kd, Qh, 7c, 2d → P0 has AA + QQ, P1 has AA + KK
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA + KK > AA + QQ
}

TEST(HandEvaluationTest, TwoPair_BothHave_SamePairs_Player0BetterKicker) {
    // Board has two pair (AA + KK), P0 has Q kicker, P1 has J kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ks, Kc, 2h → both play AA + KK, P0 kicker Q, P1 kicker J
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA + KK with Q kicker > J kicker
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, TwoPair_BothHave_SamePairs_Player1BetterKicker) {
    // Board has two pair (AA + KK), P0 has J kicker, P1 has Q kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ks, Kc, 2h → both play AA + KK, P0 kicker J, P1 kicker Q
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA + KK with Q kicker > J kicker
}

TEST(HandEvaluationTest, TwoPair_BothHave_Chop_FromBoardCards) {
    // Board has two pair (AA + KK) + Q, both players have low cards → chop
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'3'), Card(/*suit=*/'S', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'5'), Card(/*suit=*/'C', /*rank=*/'6')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ks, Kc, Qh → both play AA + KK + Q kicker
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);  // Chop: both play AA + KK + Q
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, TwoPair_BothHave_Chop_FromHoleCards) {
    // Both have same two pair using their hole cards (P0: Ah Kh, P1: Ad Kd)
    // Board doesn't pair either → both make AA + KK from their hands + board
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'H', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'D', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Ks, Qc, 7h, 2s → both have AA + KK with Q kicker
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);  // Chop: both have AA + KK + Q kicker
    EXPECT_EQ(stacks[1], 100);
}

// ==================== One Pair Tests ====================

TEST(HandEvaluationTest, Pair_Player0Wins) {
    // P0 has pocket Aces (pair), P1 has nothing
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qc, Ts, 7h, 3d → P0 has pair of Aces, P1 has nothing
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins with pair of Aces
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Pair_Player1Wins) {
    // P0 has nothing, P1 has pocket Kings (pair)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'C', /*rank=*/'2'), Card(/*suit=*/'D', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ad, Qc, Ts, 7h, 3d → P1 has pair of Kings, P0 has nothing
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins with pair of Kings
}

TEST(HandEvaluationTest, Pair_BothHave_Player0HigherPair) {
    // P0 has pocket Aces, P1 has pocket Kings
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Qd, Tc, 8s, 5h, 3d → P0 pair AA > P1 pair KK
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'T'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'8'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA > KK
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Pair_BothHave_Player1HigherPair) {
    // P0 has pocket Kings, P1 has pocket Aces
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'K'), Card(/*suit=*/'C', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Qd, Tc, 8s, 5h, 3d → P1 pair AA > P0 pair KK
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'T'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'8'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA > KK
}

TEST(HandEvaluationTest, Pair_SamePair_Player0Better1stKicker) {
    // Both pair Aces on board, P0 has K kicker, P1 has Q kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, 9c, 7h, 5d, 2s → pair of AA, P0 kicker K, P1 kicker Q
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA with K kicker > Q kicker
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Pair_SamePair_Player1Better1stKicker) {
    // Both pair Aces on board, P0 has Q kicker, P1 has K kicker
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'K')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, 9c, 7h, 5d, 2s → pair of AA, P0 kicker Q, P1 kicker K
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA with K kicker > Q kicker
}

TEST(HandEvaluationTest, Pair_SamePair_Same1stKicker_Player0Better2ndKicker) {
    // Both pair Aces on board with same top kicker K, P0 has Q as 2nd kicker, P1 has J
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'Q')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'J')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, 7h, 5d, 2s → pair AA, both have K kicker, P0 2nd kicker Q > P1 2nd kicker J
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA K Q > AA K J
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Pair_SamePair_Same1stKicker_Player1Better2ndKicker) {
    // Both pair Aces on board with same top kicker K, P0 has J as 2nd kicker, P1 has Q
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'J')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'Q')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, 7h, 5d, 2s → pair AA, both have K kicker, P1 2nd kicker Q > P0 2nd kicker J
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA K Q > AA K J
}

TEST(HandEvaluationTest, Pair_SamePair_Same1st2ndKicker_Player0Better3rdKicker) {
    // Both pair Aces on board, same 1st kicker K, same 2nd kicker Q from board
    // P0 has J as 3rd kicker, P1 has T
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'J')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'T')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, Qh, 5d, 2s → pair AA, K Q from board, P0 3rd kicker J > P1 3rd kicker T
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: AA K Q J > AA K Q T
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, Pair_SamePair_Same1st2ndKicker_Player1Better3rdKicker) {
    // Both pair Aces on board, same 1st kicker K, same 2nd kicker Q from board
    // P0 has T as 3rd kicker, P1 has J
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'T')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'J')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: As, Kc, Qh, 5d, 2s → pair AA, K Q from board, P1 3rd kicker J > P0 3rd kicker T
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: AA K Q J > AA K Q T
}

TEST(HandEvaluationTest, Pair_BothHave_Chop_FromBoardCards) {
    // Board has pair of Aces + K Q J, both players have low cards → play the board
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'3'), Card(/*suit=*/'S', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'5'), Card(/*suit=*/'C', /*rank=*/'6')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Ad, Ks, Qc, Jh → both play AA K Q J
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'K'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'J'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);  // Chop: both play AA K Q J
    EXPECT_EQ(stacks[1], 100);
}

TEST(HandEvaluationTest, Pair_BothHave_Chop_FromHoleCards) {
    // Both have pocket Aces (different suits), same board kickers → chop
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'A')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'A'), Card(/*suit=*/'C', /*rank=*/'A')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qc, Ts, 7h, 3d → both have AA with K Q T kickers
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'3'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);  // Chop: both have AA K Q T
    EXPECT_EQ(stacks[1], 100);
}

// ==================== High Card Tests ====================

TEST(HandEvaluationTest, HighCard_Player0WinsHigherTopCard) {
    // P0 has Ace high, P1 has King high (no pairs, no draws)
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qc, Ts, 7h, 2d → no pairs, P0 A-high > P1 K-high
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A K Q T 7 > K Q T 9 7
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, HighCard_Player1WinsHigherTopCard) {
    // P0 has King high, P1 has Ace high
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'A'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Kd, Qc, Ts, 7h, 2d → P1 A-high > P0 K-high
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'Q'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'T'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'7'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A K Q T 7 > K Q T 9 7
}

TEST(HandEvaluationTest, HighCard_Same1st_Player0Better2nd) {
    // Both have A-high from board, P0 has K as 2nd card, P1 has J
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, 9c, 7s, 5d, 2h → P0: A K 9 7 5, P1: A J 9 7 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A K 9 7 5 > A J 9 7 5
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, HighCard_Same1st_Player1Better2nd) {
    // Both have A-high from board, P0 has J as 2nd card, P1 has K
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'K'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, 9c, 7s, 5d, 2h → P0: A J 9 7 5, P1: A K 9 7 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'9'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A K 9 7 5 > A J 9 7 5
}

TEST(HandEvaluationTest, HighCard_Same1st2nd_Player0Better3rd) {
    // Both share A K from board, P0 has Q as 3rd card, P1 has J
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, 7s, 5d, 2h → P0: A K Q 7 5, P1: A K J 7 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A K Q 7 5 > A K J 7 5
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, HighCard_Same1st2nd_Player1Better3rd) {
    // Both share A K from board, P0 has J as 3rd card, P1 has Q
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'J'), Card(/*suit=*/'C', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'Q'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, 7s, 5d, 2h → P0: A K J 7 5, P1: A K Q 7 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'7'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A K Q 7 5 > A K J 7 5
}

TEST(HandEvaluationTest, HighCard_Same1st2nd3rd_Player0Better4th) {
    // Both share A K Q from board, P0 has T as 4th card, P1 has 9
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'T'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, Qs, 5d, 2h → P0: A K Q T 5, P1: A K Q 9 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A K Q T 5 > A K Q 9 5
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, HighCard_Same1st2nd3rd_Player1Better4th) {
    // Both share A K Q from board, P0 has 9 as 4th card, P1 has T
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'9'), Card(/*suit=*/'C', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'T'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, Qs, 5d, 2h → P0: A K Q 9 5, P1: A K Q T 5
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'5'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A K Q T 5 > A K Q 9 5
}

TEST(HandEvaluationTest, HighCard_Same1st2nd3rd4th_Player0Better5th) {
    // Both share A K Q T from board, P0 has 8 as 5th card, P1 has 7
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'8'), Card(/*suit=*/'S', /*rank=*/'3')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'7'), Card(/*suit=*/'C', /*rank=*/'4')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, Qs, Td, 2h → P0: A K Q T 8, P1: A K Q T 7
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 102);  // P0 wins: A K Q T 8 > A K Q T 7
    EXPECT_EQ(stacks[1], 98);
}

TEST(HandEvaluationTest, HighCard_Same1st2nd3rd4th_Player1Better5th) {
    // Both share A K Q T from board, P0 has 7 as 5th card, P1 has 8
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'D', /*rank=*/'7'), Card(/*suit=*/'C', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'H', /*rank=*/'8'), Card(/*suit=*/'S', /*rank=*/'3')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, Qs, Td, 2h → P0: A K Q T 7, P1: A K Q T 8
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'2'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 98);
    EXPECT_EQ(stacks[1], 102);  // P1 wins: A K Q T 8 > A K Q T 7
}

TEST(HandEvaluationTest, HighCard_Chop_PlayTheBoard) {
    // Board has A K Q T 8, both players have low cards → play the board
    std::array<Card, 2> sb_hand = {Card(/*suit=*/'H', /*rank=*/'3'), Card(/*suit=*/'S', /*rank=*/'4')};
    std::array<Card, 2> bb_hand = {Card(/*suit=*/'D', /*rank=*/'5'), Card(/*suit=*/'C', /*rank=*/'6')};
    
    PokerKit game(/*small_blind=*/1, /*big_blind=*/2, /*stack_p0=*/100, /*stack_p1=*/100, sb_hand, bb_hand);
    
    // Preflop
    game.check_or_call();
    game.check_or_call();
    
    // Board: Ah, Kc, Qs, Td, 8h → both play A K Q T 8
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'A'));
    game.deal_board(Card(/*suit=*/'C', /*rank=*/'K'));
    game.deal_board(Card(/*suit=*/'S', /*rank=*/'Q'));
    
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'D', /*rank=*/'T'));
    game.check_or_call();
    game.check_or_call();
    
    game.deal_board(Card(/*suit=*/'H', /*rank=*/'8'));
    game.check_or_call();
    game.check_or_call();
    
    EXPECT_TRUE(game.is_game_over());
    auto stacks = game.get_stacks();
    EXPECT_EQ(stacks[0], 100);  // Chop: both play A K Q T 8
    EXPECT_EQ(stacks[1], 100);
}

