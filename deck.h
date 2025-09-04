#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>

class Deck {
private:
    std::vector<Card> cards;

public:
    Deck();
    void shuffle();
    Card dealCard();
    void reset();
    int size() const;
    bool isEmpty() const;
};

#endif
