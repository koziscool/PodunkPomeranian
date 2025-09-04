#include "deck.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <stdexcept>

Deck::Deck() {
    reset();
}

void Deck::reset() {
    cards.clear();
    
    for (int s = 0; s < 4; s++) {
        for (int r = 2; r <= 14; r++) {
            cards.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
}

void Deck::shuffle() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::shuffle(cards.begin(), cards.end(), generator);
}

Card Deck::dealCard() {
    if (isEmpty()) {
        throw std::runtime_error("Cannot deal from empty deck");
    }
    
    Card card = cards.back();
    cards.pop_back();
    return card;
}

int Deck::size() const {
    return cards.size();
}

bool Deck::isEmpty() const {
    return cards.empty();
}
