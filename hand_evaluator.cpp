#include "hand_evaluator.h"
#include <algorithm>
#include <map>
#include <set>

// Define the unqualified low hand constant
const LowHandResult LO_HAND_UNQUALIFIED = {
    false,                     // qualified = false
    {15, 15, 15, 15, 15},     // values = impossibly high values (worse than any real hand)
    {},                        // bestLowHand = empty
    "No qualifying low"        // description
};

bool HandResult::operator>(const HandResult& other) const {
    if (rank != other.rank) {
        return static_cast<int>(rank) > static_cast<int>(other.rank);
    }
    return values > other.values;
}

bool HandResult::operator<(const HandResult& other) const {
    return other > *this;
}

bool HandResult::operator==(const HandResult& other) const {
    return rank == other.rank && values == other.values;
}

bool LowHandResult::operator<(const LowHandResult& other) const {
    // If neither qualifies, they tie (neither is better)
    if (!qualified && !other.qualified) return false;
    
    // Unqualified hands lose to qualified hands
    if (!qualified && other.qualified) return false;  // This loses
    if (qualified && !other.qualified) return true;   // This wins
    
    // Both qualify, compare values (lower is better for low hands)
    return values < other.values;
}

bool LowHandResult::operator==(const LowHandResult& other) const {
    return qualified == other.qualified && values == other.values;
}

HandResult HandEvaluator::evaluateHand(const std::vector<Card>& playerCards, 
                                     const std::vector<Card>& communityCards) {
    std::vector<Card> allCards = playerCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
    
    if (allCards.size() < 5) {
        // Not enough cards for a complete hand
        HandResult result;
        result.rank = HandRank::HIGH_CARD;
        result.bestHand = allCards;
        result.description = "Incomplete hand";
        return result;
    }
    
    std::vector<Card> bestFive = getBestFiveCards(allCards);
    return evaluateFiveCards(bestFive);
}

std::vector<Card> HandEvaluator::getBestFiveCards(const std::vector<Card>& allCards) {
    if (allCards.size() <= 5) {
        return allCards;
    }
    
    // Generate all combinations of 5 cards and find the best
    std::vector<Card> bestHand;
    HandResult bestResult;
    bestResult.rank = HandRank::HIGH_CARD;
    
    // Use algorithm to generate combinations
    std::vector<bool> selector(allCards.size());
    std::fill(selector.end() - 5, selector.end(), true);
    
    do {
        std::vector<Card> combination;
        for (size_t i = 0; i < allCards.size(); i++) {
            if (selector[i]) {
                combination.push_back(allCards[i]);
            }
        }
        
        HandResult result = evaluateFiveCards(combination);
        if (result > bestResult) {
            bestResult = result;
            bestHand = combination;
        }
    } while (std::next_permutation(selector.begin(), selector.end()));
    
    return bestHand;
}

HandResult HandEvaluator::evaluateFiveCards(const std::vector<Card>& cards) {
    if (cards.size() != 5) {
        HandResult result;
        result.rank = HandRank::HIGH_CARD;
        result.description = "Invalid hand size";
        return result;
    }
    
    HandResult result;
    result.bestHand = cards;
    
    // Sort cards by rank (descending)
    std::vector<Card> sortedCards = cards;
    std::sort(sortedCards.begin(), sortedCards.end(), 
              [](const Card& a, const Card& b) { return a.getRank() > b.getRank(); });
    
    bool flush = isFlush(sortedCards);
    bool straight = isStraight(sortedCards);
    auto cardCounts = getCardCounts(sortedCards);
    
    // Royal Flush
    if (flush && straight && sortedCards[0].getRank() == Rank::ACE) {
        result.rank = HandRank::ROYAL_FLUSH;
        result.values = {static_cast<int>(Rank::ACE)};
        result.description = "Royal Flush";
        return result;
    }
    
    // Straight Flush
    if (flush && straight) {
        result.rank = HandRank::STRAIGHT_FLUSH;
        // Check for wheel straight flush (A-2-3-4-5)
        if (sortedCards[0].getRank() == Rank::ACE && sortedCards[4].getRank() == Rank::TWO) {
            result.values = {1}; // Wheel straight flush ranks as lowest (1-high for comparison)
            result.description = "Straight Flush: 5 high";
        } else {
            result.values = {static_cast<int>(sortedCards[0].getRank())};
            result.description = "Straight Flush: " + getRankName(sortedCards[0].getRank()) + " high";
        }
        return result;
    }
    
    // Four of a Kind
    if (cardCounts[0].first == 4) {
        result.rank = HandRank::FOUR_OF_A_KIND;
        result.values = {static_cast<int>(cardCounts[0].second), static_cast<int>(cardCounts[1].second)};
        
        // Reorder cards: quads first, then kicker
        std::vector<Card> reorderedCards;
        
        // Find the quads first
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[0].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Then the kicker
        for (const auto& card : sortedCards) {
            if (card.getRank() != cardCounts[0].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Show the reordered 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < reorderedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += reorderedCards[i].toString();
        }
        result.description = "Four of a Kind: " + getRankName(cardCounts[0].second) + "s (" + handStr + ")";
        return result;
    }
    
    // Full House
    if (cardCounts[0].first == 3 && cardCounts[1].first == 2) {
        result.rank = HandRank::FULL_HOUSE;
        result.values = {static_cast<int>(cardCounts[0].second), static_cast<int>(cardCounts[1].second)};
        
        // Reorder cards: trips first, then pair
        std::vector<Card> reorderedCards;
        
        // Find the trips first
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[0].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Then the pair
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[1].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Show the reordered 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < reorderedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += reorderedCards[i].toString();
        }
        result.description = "Full House: " + getRankName(cardCounts[0].second) + "s full of " + getRankName(cardCounts[1].second) + "s (" + handStr + ")";
        return result;
    }
    
    // Flush
    if (flush) {
        result.rank = HandRank::FLUSH;
        for (const auto& card : sortedCards) {
            result.values.push_back(static_cast<int>(card.getRank()));
        }
        
        // Show the complete 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < sortedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += sortedCards[i].toString();
        }
        result.description = "Flush: " + getRankName(sortedCards[0].getRank()) + " high (" + handStr + ")";
        return result;
    }
    
    // Straight
    if (straight) {
        result.rank = HandRank::STRAIGHT;
        // Check for wheel straight (A-2-3-4-5)
        if (sortedCards[0].getRank() == Rank::ACE && sortedCards[4].getRank() == Rank::TWO) {
            result.values = {1}; // Wheel ranks as lowest straight (1-high for comparison)
            result.description = "Straight: 5 high";
        } else {
            result.values = {static_cast<int>(sortedCards[0].getRank())};
            result.description = "Straight: " + getRankName(sortedCards[0].getRank()) + " high";
        }
        return result;
    }
    
    // Three of a Kind
    if (cardCounts[0].first == 3) {
        result.rank = HandRank::THREE_OF_A_KIND;
        result.values = {static_cast<int>(cardCounts[0].second), static_cast<int>(cardCounts[1].second), static_cast<int>(cardCounts[2].second)};
        
        // Reorder cards: trips first, then kickers in descending order
        std::vector<Card> reorderedCards;
        std::vector<Card> kickers;
        
        // Find the trips and kickers
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[0].second) {
                reorderedCards.push_back(card);
            } else {
                kickers.push_back(card);
            }
        }
        
        // Add kickers (already sorted in descending order)
        reorderedCards.insert(reorderedCards.end(), kickers.begin(), kickers.end());
        
        // Show the reordered 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < reorderedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += reorderedCards[i].toString();
        }
        result.description = "Three of a Kind: " + getRankName(cardCounts[0].second) + "s, " + 
                            getRankName(cardCounts[1].second) + " " + 
                            getRankName(cardCounts[2].second) + " kickers (" + handStr + ")";
        return result;
    }
    
    // Two Pair
    if (cardCounts[0].first == 2 && cardCounts[1].first == 2) {
        result.rank = HandRank::TWO_PAIR;
        result.values = {static_cast<int>(cardCounts[0].second), static_cast<int>(cardCounts[1].second), static_cast<int>(cardCounts[2].second)};
        
        // Reorder cards: higher pair, lower pair, then kicker
        std::vector<Card> reorderedCards;
        std::vector<Card> kickers;
        
        // Find higher pair first
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[0].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Then lower pair
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[1].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Then kicker
        for (const auto& card : sortedCards) {
            if (card.getRank() != cardCounts[0].second && card.getRank() != cardCounts[1].second) {
                reorderedCards.push_back(card);
            }
        }
        
        // Show the reordered 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < reorderedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += reorderedCards[i].toString();
        }
        result.description = "Two Pair: " + getRankName(cardCounts[0].second) + "s over " + 
                            getRankName(cardCounts[1].second) + "s, " + 
                            getRankName(cardCounts[2].second) + " kicker (" + handStr + ")";
        return result;
    }
    
    // One Pair
    if (cardCounts[0].first == 2) {
        result.rank = HandRank::ONE_PAIR;
        result.values = {static_cast<int>(cardCounts[0].second), static_cast<int>(cardCounts[1].second), static_cast<int>(cardCounts[2].second), static_cast<int>(cardCounts[3].second)};
        
        // Reorder cards: pair first, then kickers in descending order
        std::vector<Card> reorderedCards;
        std::vector<Card> kickers;
        
        // Find the pair and kickers
        for (const auto& card : sortedCards) {
            if (card.getRank() == cardCounts[0].second) {
                reorderedCards.push_back(card);
            } else {
                kickers.push_back(card);
            }
        }
        
        // Add kickers (already sorted in descending order)
        reorderedCards.insert(reorderedCards.end(), kickers.begin(), kickers.end());
        
        // Show the reordered 5-card hand
        std::string handStr = "";
        for (size_t i = 0; i < reorderedCards.size(); i++) {
            if (i > 0) handStr += " ";
            handStr += reorderedCards[i].toString();
        }
        result.description = "One Pair: " + getRankName(cardCounts[0].second) + "s, " + 
                            getRankName(cardCounts[1].second) + " " + 
                            getRankName(cardCounts[2].second) + " " +
                            getRankName(cardCounts[3].second) + " kickers (" + handStr + ")";
        return result;
    }
    
    // High Card
    result.rank = HandRank::HIGH_CARD;
    for (const auto& card : sortedCards) {
        result.values.push_back(static_cast<int>(card.getRank()));
    }
    
    // Show the complete 5-card hand
    std::string handStr = "";
    for (size_t i = 0; i < sortedCards.size(); i++) {
        if (i > 0) handStr += " ";
        handStr += sortedCards[i].toString();
    }
    result.description = "High Card: " + getRankName(sortedCards[0].getRank()) + " " +
                        getRankName(sortedCards[1].getRank()) + " " +
                        getRankName(sortedCards[2].getRank()) + " " +
                        getRankName(sortedCards[3].getRank()) + " " +
                        getRankName(sortedCards[4].getRank()) + " (" + handStr + ")";
    
    return result;
}

bool HandEvaluator::isFlush(const std::vector<Card>& cards) {
    Suit firstSuit = cards[0].getSuit();
    for (const auto& card : cards) {
        if (card.getSuit() != firstSuit) {
            return false;
        }
    }
    return true;
}

bool HandEvaluator::isStraight(const std::vector<Card>& cards) {
    std::vector<int> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<int>(card.getRank()));
    }
    std::sort(ranks.begin(), ranks.end(), std::greater<int>());
    
    // Check for normal straight
    for (size_t i = 1; i < ranks.size(); i++) {
        if (ranks[i-1] - ranks[i] != 1) {
            // Check for A-2-3-4-5 straight (wheel)
            if (ranks[0] == 14 && ranks[1] == 5 && ranks[2] == 4 && ranks[3] == 3 && ranks[4] == 2) {
                return true;
            }
            return false;
        }
    }
    return true;
}

std::vector<std::pair<int, Rank>> HandEvaluator::getCardCounts(const std::vector<Card>& cards) {
    std::map<Rank, int> counts;
    for (const auto& card : cards) {
        counts[card.getRank()]++;
    }
    
    std::vector<std::pair<int, Rank>> countPairs;
    for (const auto& pair : counts) {
        countPairs.emplace_back(pair.second, pair.first);
    }
    
    // Sort by count (descending), then by rank (descending)
    std::sort(countPairs.begin(), countPairs.end(), 
              [](const std::pair<int, Rank>& a, const std::pair<int, Rank>& b) {
                  if (a.first != b.first) return a.first > b.first;
                  return static_cast<int>(a.second) > static_cast<int>(b.second);
              });
    
    return countPairs;
}

std::string HandEvaluator::getRankName(Rank rank) {
    switch (rank) {
        case Rank::TWO:   return "2";
        case Rank::THREE: return "3";
        case Rank::FOUR:  return "4";
        case Rank::FIVE:  return "5";
        case Rank::SIX:   return "6";
        case Rank::SEVEN: return "7";
        case Rank::EIGHT: return "8";
        case Rank::NINE:  return "9";
        case Rank::TEN:   return "Ten";
        case Rank::JACK:  return "Jack";
        case Rank::QUEEN: return "Queen";
        case Rank::KING:  return "King";
        case Rank::ACE:   return "Ace";
    }
    return "";
}

std::string HandEvaluator::getSuitName(Suit suit) {
    switch (suit) {
        case Suit::CLUBS:    return "Clubs";
        case Suit::DIAMONDS: return "Diamonds";
        case Suit::HEARTS:   return "Hearts";
        case Suit::SPADES:   return "Spades";
    }
    return "";
}

LowHandResult HandEvaluator::evaluate5CardsForLowA5(const std::vector<Card>& fiveCards) {
    if (fiveCards.size() != 5) {
        return LO_HAND_UNQUALIFIED;
    }
    
    // Convert all ranks to low values (A=1, 2=2, ..., K=13)
    std::vector<int> ranks;
    for (const auto& card : fiveCards) {
        int lowValue;
        switch (card.getRank()) {
            case Rank::ACE:   lowValue = 1; break;
            case Rank::TWO:   lowValue = 2; break;
            case Rank::THREE: lowValue = 3; break;
            case Rank::FOUR:  lowValue = 4; break;
            case Rank::FIVE:  lowValue = 5; break;
            case Rank::SIX:   lowValue = 6; break;
            case Rank::SEVEN: lowValue = 7; break;
            case Rank::EIGHT: lowValue = 8; break;
            case Rank::NINE:  lowValue = 9; break;
            case Rank::TEN:   lowValue = 10; break;
            case Rank::JACK:  lowValue = 11; break;
            case Rank::QUEEN: lowValue = 12; break;
            case Rank::KING:  lowValue = 13; break;
        }
        ranks.push_back(lowValue);
    }
    
    // Sort ranks
    std::sort(ranks.begin(), ranks.end());
    
    // Count pairs/trips/quads - more pairs = worse for low
    std::map<int, int> counts;
    for (int rank : ranks) {
        counts[rank]++;
    }
    
    // Create comparison values - pairs/trips make the hand much worse
    std::vector<int> compareValues;
    
    // First, add a "hand type" value (lower is better for low)
    // No pairs = 0, one pair = 100, two pair = 200, trips = 300, etc.
    int handType = 0;
    for (const auto& count : counts) {
        if (count.second == 2) handType += 100;      // pair
        else if (count.second == 3) handType += 300; // trips  
        else if (count.second == 4) handType += 500; // quads
    }
    compareValues.push_back(handType);
    
    // Then add ranks in order of importance for low hands
    // For paired hands: pair rank(s) first (higher pair = worse), then kickers
    // For no-pair hands: just the ranks high to low
    
    if (handType == 0) {
        // No pairs - just add ranks from high to low
        for (int i = 4; i >= 0; i--) {
            compareValues.push_back(ranks[i]);
        }
    } else {
        // Has pairs - add pair ranks first (higher pairs = worse for low)
        std::vector<int> pairRanks;
        std::vector<int> kickers;
        
        for (const auto& count : counts) {
            if (count.second >= 2) {
                // Add pair rank multiple times based on count for proper ordering
                for (int j = 0; j < count.second; j++) {
                    pairRanks.push_back(count.first);
                }
            } else {
                kickers.push_back(count.first);
            }
        }
        
        // Sort pair ranks high to low (higher pairs are worse for low)
        std::sort(pairRanks.rbegin(), pairRanks.rend());
        // Sort kickers high to low
        std::sort(kickers.rbegin(), kickers.rend());
        
        // Add pair ranks first, then kickers
        for (int rank : pairRanks) {
            compareValues.push_back(rank);
        }
        for (int rank : kickers) {
            compareValues.push_back(rank);
        }
    }
    
    LowHandResult result;
    result.bestLowHand = fiveCards;
    result.values = compareValues;
    result.qualified = true;  // Always true at this level - qualification checked at poker game level
    
    // Create description showing the hand from high to low card
    std::string desc = "";
    for (int i = 4; i >= 0; i--) {  // Show highest to lowest
        if (desc.length() > 0) desc += "-";
        if (ranks[i] == 1) desc += "A";
        else if (ranks[i] == 11) desc += "J";
        else if (ranks[i] == 12) desc += "Q"; 
        else if (ranks[i] == 13) desc += "K";
        else desc += std::to_string(ranks[i]);
    }
    desc += " low";
    result.description = desc;
    
    return result;
}

LowHandResult HandEvaluator::evaluateLowHand(const std::vector<Card>& playerCards, 
                                            const std::vector<Card>& communityCards) {
    // Combine all available cards
    std::vector<Card> allCards = playerCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
    
    if (allCards.size() <= 5) {
        return evaluate5CardsForLowA5(allCards);
    }
    
    // Generate all combinations of 5 cards and find the best for low
    LowHandResult bestLowResult = LO_HAND_UNQUALIFIED;
    
    // Use algorithm to generate combinations  
    std::vector<bool> selector(allCards.size());
    std::fill(selector.end() - 5, selector.end(), true);
    
    do {
        std::vector<Card> combination;
        for (size_t i = 0; i < allCards.size(); ++i) {
            if (selector[i]) {
                combination.push_back(allCards[i]);
            }
        }
        
        LowHandResult result = evaluate5CardsForLowA5(combination);
        if (result < bestLowResult) {
            bestLowResult = result;
        }
        
    } while (std::next_permutation(selector.begin(), selector.end()));
    
    return bestLowResult;
} 