#include "player.h"
#include "hand_history.h"
#include "variants.h"
#include <iostream>
#include <algorithm>
#include <ctime>

Player::Player(const std::string& playerName, int startingChips, int id, PlayerPersonality playerPersonality) 
    : name(playerName), chips(startingChips), cardsAtStartOfStreet(0), currentBet(0), inFor(0), folded(false), allIn(false), 
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

void Player::markStartOfStreet() {
    cardsAtStartOfStreet = hand.size();
}

void Player::showStudHandWithNew() const {
    for (size_t i = 0; i < hand.size(); i++) {
        bool isNewCard = (static_cast<int>(i) >= cardsAtStartOfStreet);
        
        if (cardsFaceUp[i]) {
            std::cout << hand[i].toString();
            if (isNewCard) std::cout << "*"; // Mark new cards with *
            std::cout << " ";
        } else {
            std::cout << "[" << hand[i].toString() << "]";
            if (isNewCard) std::cout << "*"; // Mark new hole cards too
            std::cout << " ";
        }
    }
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

void Player::setInFor(int amount) {
    inFor = amount;
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
PlayerAction Player::makeDecision(const HandHistory& history, int callAmount, bool canCheck, const VariantInfo* variant, int betCount) const {
    // Check bet cap for limit games (4-bet cap)
    bool canRaise = true;
    if (variant && variant->bettingStruct == BETTINGSTRUCTURE_LIMIT && betCount >= 4) {
        canRaise = false; // 4-bet cap reached
    }
    
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
            // Strong hand - almost always raise for aggressive players
            if (personality == PlayerPersonality::TIGHT_AGGRESSIVE || 
                personality == PlayerPersonality::LOOSE_AGGRESSIVE) {
                
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < 0.8 && canRaise) { // 80% chance to raise with strong hand
                    return PlayerAction::RAISE;
                }
            }
            return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
        }
        
        // Medium strength hands - be more aggressive
        if (handStrength > 0.5) {
            // Sometimes raise with medium-good hands
            if (personality == PlayerPersonality::LOOSE_AGGRESSIVE) {
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < 0.4 && callAmount == 0 && canRaise) { // 40% chance to bet when checking is free
                    return PlayerAction::RAISE;
                }
            }
            if (callAmount == 0) return PlayerAction::CHECK;
            if (potOdds > 0.25) return PlayerAction::CALL;
            return PlayerAction::FOLD;
        }
        
        // Playable hands
        if (handStrength > 0.3) {
            if (callAmount == 0) return PlayerAction::CHECK;
            if (potOdds > 0.3) return PlayerAction::CALL;
            return PlayerAction::FOLD;
        }
        
        // Weak hands
        if (canCheck) return PlayerAction::CHECK;
        return PlayerAction::FOLD;
    }
    
    // Post-flop decision making (more aggressive)
    if (shouldFoldToAggression(history, callAmount)) {
        return PlayerAction::FOLD;
    }
    
    // Check if we're on turn/river for more aggressive play
    bool isLateStreet = (history.getCurrentRound() == HandHistoryRound::TURN || 
                        history.getCurrentRound() == HandHistoryRound::RIVER);
    
    // SUPER AGGRESSIVE on turn/river for testing limit betting structure
    if (isLateStreet && variant && variant->variantName == "Omaha Hi-Lo") {
        // On turn/river in Omaha, make everyone extremely aggressive
        if (handStrength > 0.4) { // Much lower threshold
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < 0.98 && canRaise) { // 98% chance to raise!
                return PlayerAction::RAISE;
            }
            return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
        }
        // Even mediocre hands will often bet/call on turn/river
        if (handStrength > 0.25) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (callAmount == 0 && dist(rng) < 0.95 && canRaise) {
                return PlayerAction::RAISE; // 95% chance to bet
            }
            if (callAmount > 0 && dist(rng) < 0.90) {
                return PlayerAction::CALL; // 90% chance to call
            }
        }
    }
    
    // Very strong hands - bet/raise aggressively  
    if (handStrength > 0.7) {
        if (personality == PlayerPersonality::TIGHT_AGGRESSIVE || 
            personality == PlayerPersonality::LOOSE_AGGRESSIVE) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double raiseChance = isLateStreet ? 0.95 : 0.85; // Much more aggressive
            if (dist(rng) < raiseChance && canRaise) {
                return PlayerAction::RAISE;
            }
        }
        return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
    }
    
    // Good hands - value bet or call
    if (handStrength > 0.5) {
        if (callAmount == 0) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double betChance = isLateStreet ? 0.9 : 0.7; // Much more aggressive on turn/river
            if (dist(rng) < betChance && canRaise) {
                return PlayerAction::RAISE;
            }
        }
        return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
    }
    
    // Special case for Omaha turn/river - make weaker hands call more often
    if (isLateStreet && variant && variant->variantName == "Omaha Hi-Lo") {
        // Even weak hands will call on turn/river in Omaha (for testing)
        if (handStrength > 0.15 && callAmount > 0) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < 0.85) { // 85% chance to call even with weak hands
                return PlayerAction::CALL;
            }
        }
        // Bluff very often on turn/river
        if (callAmount == 0 && canRaise) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < 0.80) { // 80% chance to bluff bet
                return PlayerAction::RAISE;
            }
        }
    }
    
    // Medium hands 
    if (handStrength > 0.4 || potOdds > 0.25) {
        return callAmount > 0 ? PlayerAction::CALL : PlayerAction::CHECK;
    }
    
    // Bluff occasionally with weak hands (more on turn/river)
    if (callAmount == 0 && canRaise) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double bluffChance = isLateStreet ? 0.3 : 0.1; // Much more bluffing on turn/river
        if (dist(rng) < bluffChance) {
            return PlayerAction::RAISE;
        }
    }
    
    if (canCheck) {
        return PlayerAction::CHECK;
    }
    
    return PlayerAction::FOLD;
}

int Player::calculateRaiseAmount(const HandHistory& /* history */, int currentBet, const VariantInfo& variant, UnifiedBettingRound currentRound) const {
    if (variant.bettingStruct == BETTINGSTRUCTURE_LIMIT) {
        // Limit poker logic
        if (variant.gameStruct == GAMESTRUCTURE_STUD) {
            // Seven Card Stud limit logic
            int bringIn = variant.betSizes[1]; 
            int smallBet = variant.betSizes[2];
            int bigBet = variant.betSizes[3];
            
            if (currentBet == bringIn) {
                // Complete bring-in to small bet
                return smallBet;
            } else if (currentRound == UNIFIED_PRE_FLOP || currentRound == UNIFIED_FLOP) {
                // 3rd and 4th street: small bet rounds
                return currentBet + smallBet;
            } else {
                // 5th street and beyond: big bet rounds
                return currentBet + bigBet;
            }
        } else {
            // Board games (Hold'em, Omaha) limit logic
            int smallBet = variant.betSizes[2];
            int bigBet = variant.betSizes[3];
            
            if (currentRound == UNIFIED_PRE_FLOP || currentRound == UNIFIED_FLOP) {
                // Pre-flop and flop: small bet rounds
                return currentBet + smallBet;
            } else {
                // Turn and river: big bet rounds
                return currentBet + bigBet;
            }
        }
    } else {
        // No-limit poker logic (simplified for now)
        double handStrength = evaluateHandStrength();
        
        // Base raise size on current bet and hand strength
        int baseRaise = std::max(currentBet * 2, 50); // Minimum raise of 50
        
        // Adjust for personality and hand strength
        baseRaise = static_cast<int>(baseRaise * (0.5 + handStrength));
        
        // Don't bet more than we have
        return std::min(baseRaise, chips);
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
    // Never fold when we can check for free
    if (callAmount == 0) {
        return false;
    }
    
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