#ifndef CARD_HPP
#define CARD_HPP

class Card {
public:
    Card(char suit, char rank);

    bool operator<(const Card& other) const;
    bool operator>(const Card& other) const;
    bool operator<=(const Card& other) const;
    bool operator>=(const Card& other) const;
    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;

    char suit;
    char rank;

private:
    int rank_value() const;
};

#endif // CARD_HPP

