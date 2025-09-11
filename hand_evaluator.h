#ifndef HAND_EVALUATOR_H
#define HAND_EVALUATOR_H

#include "card.h"
#include <vector>
#include <string>

enum class HandRank {
    HIGH_CARD = 1,
    ONE_PAIR = 2,
    TWO_PAIR = 3,
    THREE_OF_A_KIND = 4,
    STRAIGHT = 5,
    FLUSH = 6,
    FULL_HOUSE = 7,
    FOUR_OF_A_KIND = 8,
    STRAIGHT_FLUSH = 9,
    ROYAL_FLUSH = 10
};

struct HandResult {
    HandRank rank;
    std::vector<int> values;  // For tie-breaking
    std::vector<Card> bestHand;
    std::string description;
    
    bool operator>(const HandResult& other) const;
    bool operator<(const HandResult& other) const;
    bool operator==(const HandResult& other) const;
};

struct LowHandResult {
    bool qualified;  // true if qualifies for low (8-or-better)
    std::vector<int> values;  // For tie-breaking (lower values win)
    std::vector<Card> bestLowHand;
    std::string description;
    
    bool operator<(const LowHandResult& other) const;  // Lower is better for low hands
    bool operator==(const LowHandResult& other) const;
};

// Constant representing an unqualified low hand - loses to any qualified hand, ties with other unqualified
extern const LowHandResult LO_HAND_UNQUALIFIED;

class HandEvaluator {
public:
    static HandResult evaluateHand(const std::vector<Card>& playerCards, 
                                 const std::vector<Card>& communityCards);
    
    static LowHandResult evaluate5CardsForLowA5(const std::vector<Card>& fiveCards);
    static LowHandResult evaluateLowHand(const std::vector<Card>& playerCards, 
                                       const std::vector<Card>& communityCards);
    
    static std::string getRankName(Rank rank);
    static std::string getSuitName(Suit suit);
    
private:
    static HandResult evaluateFiveCards(const std::vector<Card>& cards);
    static bool isFlush(const std::vector<Card>& cards);
    static bool isStraight(const std::vector<Card>& cards);
    static std::vector<std::pair<int, Rank>> getCardCounts(const std::vector<Card>& cards);
    static std::vector<Card> getBestFiveCards(const std::vector<Card>& allCards);
    static std::string getHandDescription(const HandResult& result);
};

#endif 