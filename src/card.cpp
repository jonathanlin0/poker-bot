#include "../include/card.hpp"
#include <stdexcept>

Card::Card(char suit, char rank)
    : suit(suit),
      rank(rank) {
    if (suit != 'C' && suit != 'S' && suit != 'D' && suit != 'H') {
        throw std::runtime_error("Invalid suit: must be 'C', 'S', 'D', or 'H'");
    }
    if (rank != 'A' && rank != '2' && rank != '3' && rank != '4' && rank != '5' &&
        rank != '6' && rank != '7' && rank != '8' && rank != '9' && rank != 'T' &&
        rank != 'J' && rank != 'Q' && rank != 'K') {
        throw std::runtime_error("Invalid rank: must be 'A', '2'-'9', 'T', 'J', 'Q', or 'K'");
    }
}

int Card::rank_value() const {
    // Ascending order: 2 < 3 < 4 < 5 < 6 < 7 < 8 < 9 < T < J < Q < K < A
    switch (rank) {
        case '2': return 0;
        case '3': return 1;
        case '4': return 2;
        case '5': return 3;
        case '6': return 4;
        case '7': return 5;
        case '8': return 6;
        case '9': return 7;
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
        default: return -1;
    }
}

bool Card::operator<(const Card& other) const {
    return rank_value() < other.rank_value();
}

bool Card::operator>(const Card& other) const {
    return rank_value() > other.rank_value();
}

bool Card::operator<=(const Card& other) const {
    return rank_value() <= other.rank_value();
}

bool Card::operator>=(const Card& other) const {
    return rank_value() >= other.rank_value();
}

bool Card::operator==(const Card& other) const {
    return rank_value() == other.rank_value();
}

bool Card::operator!=(const Card& other) const {
    return rank_value() != other.rank_value();
}

