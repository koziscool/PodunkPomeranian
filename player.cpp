#include "player.h"
#include "hand_history.h"
#include <iostream>
#include <algorithm>
#include <ctime>

Player::Player(const std::string& playerName, int startingChips, int id, PlayerPersonality playerPersonality) 
    : name(playerName), chips(startingChips), currentBet(0), inFor(0), folded(false), allIn(false), 
      personality(playerPersonality), playerId(id), rng(std::time(nullptr) + id) {
}

const std::string& Player::getName() const {
    return name;
}

int Player::getChips() const {
    return chips;
}

int Player::getPlayerId() const {
    return playerId;
}

const std::vector<Card>& Player::getHand() const {
    return hand;
}

int Player::getCurrentBet() const {
    return currentBet;
}

int Player::getInFor() const {
    return inFor;
}

bool Player::hasFolded() const {
    return folded;
}

bool Player::isAllIn() const {
    return allIn;
}

int Player::getHandSize() const {
    return hand.size();
}

void Player::addCard(const Card& card) {
    hand.push_back(card);
    cardsFaceUp.push_back(false); // Default to face down for hold'em compatibility
}

void Player::addCard(const Card& card, bool faceUp) {
    hand.push_back(card);
    cardsFaceUp.push_back(faceUp);
}

void Player::clearHand() {
    hand.clear();
    cardsFaceUp.clear();
}

void Player::showHand() const {
    for (const auto& card : hand) {
        std::cout << card.toString() << " ";
    }
}

void Player::showStudHand() const {
    std::cout << name << ": ";
    for (size_t i = 0; i < hand.size(); i++) {
        if (cardsFaceUp[i]) {
            std::cout << hand[i].toString() << " ";
        } else {
            std::cout << "XX ";
        }
    }
}

std::vector<Card> Player::getUpCards() const {
    std::vector<Card> upCards;
    for (size_t i = 0; i < hand.size(); i++) {
        if (cardsFaceUp[i]) {
            upCards.push_back(hand[i]);
        }
    }
    return upCards;
}

Card Player::getLowestUpCard() const {
    std::vector<Card> upCards = getUpCards();
    if (upCards.empty()) {
        // Return an invalid card if no up cards
        return Card(Suit::HEARTS, Rank::ACE); // This should be handled by caller
    }
    
    Card lowest = upCards[0];
    for (const auto& card : upCards) {
        if (static_cast<int>(card.getRank()) < static_cast<int>(lowest.getRank())) {
            lowest = card;
        }
    }
    return lowest;
}

void Player::addChips(int amount) {
    chips += amount;
}

void Player::deductChips(int amount) {
    chips = std::max(0, chips - amount);
}

bool Player::canAfford(int amount) const {
    return chips >= amount;
}

PlayerAction Player::fold() {
    folded = true;
    return PlayerAction::FOLD;
}

PlayerAction Player::check() {
    return PlayerAction::CHECK;
}

PlayerAction Player::call(int callAmount) {
    int additionalAmount = callAmount - currentBet;
    if (additionalAmount >= chips) {
        return goAllIn();
    }
    
    addToInFor(additionalAmount);
    currentBet = callAmount;
    return PlayerAction::CALL;
}

PlayerAction Player::raise(int raiseAmount) {
    int additionalAmount = raiseAmount - currentBet;
    if (additionalAmount >= chips) {
        return goAllIn();
    }
    
    addToInFor(additionalAmount);
    currentBet = raiseAmount;
    return PlayerAction::RAISE;
}

PlayerAction Player::goAllIn() {
    int allInAmount = chips;
    addToInFor(allInAmount);
    currentBet += allInAmount;
    allIn = true;
    return PlayerAction::ALL_IN;
}

void Player::resetBet() {
    currentBet = 0;
}

void Player::setBet(int amount) {
    currentBet = amount;
}

void Player::addToInFor(int amount) {
    deductChips(amount);
    inFor += amount;
}

void Player::resetInFor() {
    inFor = 0;
}

void Player::resetForNewHand() {
    hand.clear();
    cardsFaceUp.clear();
    currentBet = 0;
    inFor = 0;
    folded = false;
    allIn = false;
}

void Player::showStatus(bool showCards) const {
    std::cout << std::setw(15) << name
              << " | Chips: " << std::setw(6) << chips
              << " | Bet: " << std::setw(4) << inFor;

    if (showCards) {
        std::cout << " | Cards: ";
        if (hand.size() > 0) {
            std::string cardStr = "";
            for (const auto& card : hand) {
                cardStr += card.toString() + " ";
            }
            std::cout << std::setw(12) << std::left << cardStr << std::right;
        } else {
            std::cout << std::setw(12) << "(none)";
        }
    }
    if (folded) {
        std::cout << " | FOLDED";
    } else if (allIn) {
        std::cout << " | ALL-IN";
    }
    std::cout << std::endl;
}

// Decision making implementation
PlayerAction Player::makeDecision(const HandHistory& history, int callAmount, bool canCheck) const {
    // If we can't afford the call amount, go all-in or fold
    if (callAmount >= chips) {
        if (chips <= 50) { // Small stack, might as well try
            return PlayerAction::ALL_IN;
        } else {
            return PlayerAction::FOLD;
        }
    }
    
    // Basic decision logic based on personality and simple heuristics
    double handStrength = evaluateHandStrength();
    double potOdds = calculatePotOdds(callAmount, history.getCurrentPot());
    
    // Pre-flop decision making
    if (history.getCurrentRound() == HandHistoryRound::PRE_FLOP) {
        // If we can check for free, never fold - at minimum check
        if (canCheck) {
            if (!isHandPlayable() || handStrength < 0.3) {
                return PlayerAction::CHECK; // Free to see more cards
            }
            // Strong enough hand and free to check - might raise
        } else {
            // We have to put in money - check if hand is playable
            if (!isHandPlayable()) {
                return PlayerAction::FOLD;
            }
        }
        
        // Call or raise based on hand strength and personality
        if (handStrength > 0.7) {
            // Strong hand - consider raising
            if (personality == PlayerPersonality::TIGHT_AGGRESSIVE || 
                personality == PlayerPersonality::LOOSE_AGGRESSIVE) {
                
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < 0.6) { // 60% chance to raise with strong hand
                    return PlayerAction::RAISE;
                }
            }
            return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
        }
        
        // Medium strength hands
        if (handStrength > 0.4) {
            if (callAmount == 0) return PlayerAction::CHECK;
            if (potOdds > 0.3) return PlayerAction::CALL;
            return PlayerAction::FOLD;
        }
        
        // Weak hands
        if (canCheck) return PlayerAction::CHECK;
        return PlayerAction::FOLD;
    }
    
    // Post-flop decision making (simplified)
    if (shouldFoldToAggression(history, callAmount)) {
        return PlayerAction::FOLD;
    }
    
    if (handStrength > 0.8 && shouldBluff(history)) {
        return PlayerAction::RAISE;
    }
    
    if (handStrength > 0.5 || potOdds > 0.25) {
        return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
    }
    
    if (canCheck) {
        return PlayerAction::CHECK;
    }
    
    return PlayerAction::FOLD;
}

int Player::calculateRaiseAmount(const HandHistory& history, int currentBet) const {
    // TEMPORARY FIX: For limit games, use fixed raise increments
    // TODO: Pass game variant info to player so it knows betting structure
    
    if (currentBet == 10) {
        // Bring-in situation: complete to small bet
        return 20;
    } else if (currentBet == 20 || currentBet == 0) {
        // Small bet round: raise by small bet increment
        return currentBet + 20;
    } else {
        // Assume big bet round: raise by big bet increment  
        return currentBet + 40;
    }
}

// Decision helper implementations
double Player::evaluateHandStrength() const {
    if (hand.empty()) return 0.0;
    
    // Very basic hand strength evaluation
    // This is a placeholder - real evaluation would depend on the variant
    
    if (hand.size() < 2) return 0.1;
    
    // Look for pairs, high cards, suited cards
    std::vector<Rank> ranks;
    std::vector<Suit> suits;
    
    for (const Card& card : hand) {
        ranks.push_back(card.getRank());
        suits.push_back(card.getSuit());
    }
    
    std::sort(ranks.begin(), ranks.end());
    
    // Check for pairs
    for (size_t i = 1; i < ranks.size(); i++) {
        if (ranks[i] == ranks[i-1]) {
            return 0.7; // Pair is decent
        }
    }
    
    // High card evaluation
    int highCardValue = static_cast<int>(ranks.back());
    if (highCardValue >= static_cast<int>(Rank::JACK)) {
        return 0.4 + (highCardValue - static_cast<int>(Rank::JACK)) * 0.1;
    }
    
    // Check for suited cards
    if (hand.size() >= 2 && suits[0] == suits[1]) {
        return 0.3; // Suited has potential
    }
    
    return 0.2; // Weak hand
}

double Player::calculatePotOdds(int callAmount, int potSize) const {
    if (callAmount <= 0) return 1.0; // No cost to see more cards
    return static_cast<double>(potSize) / (potSize + callAmount);
}

bool Player::shouldFoldToAggression(const HandHistory& history, int callAmount) const {
    // Check if there was recent aggressive action
    auto recentActions = history.getActionsForRound(history.getCurrentRound());
    
    bool heavyAggression = false;
    for (const auto& action : recentActions) {
        if (action.actionType == ActionType::RAISE && action.amount > callAmount * 2) {
            heavyAggression = true;
            break;
        }
    }
    
    if (heavyAggression && evaluateHandStrength() < 0.6) {
        return personality == PlayerPersonality::TIGHT_PASSIVE || 
               personality == PlayerPersonality::TIGHT_AGGRESSIVE;
    }
    
    return false;
}

bool Player::shouldBluff(const HandHistory& history) const {
    // Simple bluff logic - only aggressive personalities bluff
    if (personality != PlayerPersonality::LOOSE_AGGRESSIVE && 
        personality != PlayerPersonality::TIGHT_AGGRESSIVE) {
        return false;
    }
    
    // Don't bluff if many players in hand
    auto currentRoundActions = history.getActionsForRound(history.getCurrentRound());
    int activePlayers = 0;
    for (const auto& action : currentRoundActions) {
        if (action.actionType != ActionType::FOLD) {
            activePlayers++;
        }
    }
    
    if (activePlayers > 2) return false;
    
    // Random bluff frequency
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng) < 0.2; // 20% bluff frequency
}

bool Player::isHandPlayable() const {
    if (hand.size() < 2) return false;
    
    // Very basic preflop hand selection for Hold'em/Omaha
    std::vector<Rank> ranks;
    std::vector<Suit> suits;
    
    for (const Card& card : hand) {
        ranks.push_back(card.getRank());
        suits.push_back(card.getSuit());
    }
    
    // Always play pairs
    std::sort(ranks.begin(), ranks.end());
    for (size_t i = 1; i < ranks.size(); i++) {
        if (ranks[i] == ranks[i-1]) {
            return true;
        }
    }
    
    // Play high cards
    int highCard = static_cast<int>(ranks.back());
    if (highCard >= static_cast<int>(Rank::JACK)) {
        return true;
    }
    
    // Suited cards have more potential
    if (hand.size() >= 2 && suits[0] == suits[1] && highCard >= static_cast<int>(Rank::TEN)) {
        return true;
    }
    
    // Tight players fold more hands
    if (personality == PlayerPersonality::TIGHT_PASSIVE || 
        personality == PlayerPersonality::TIGHT_AGGRESSIVE) {
        return highCard >= static_cast<int>(Rank::QUEEN);
    }
    
    return highCard >= static_cast<int>(Rank::TEN);
} 