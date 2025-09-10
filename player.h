#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <vector>
#include <string>
#include <iomanip>
#include <random>

class HandHistory; // Forward declaration
struct VariantInfo; // Forward declaration

enum class PlayerPersonality {
    TIGHT_PASSIVE,   // Plays few hands, rarely raises
    TIGHT_AGGRESSIVE, // Plays few hands, bets/raises with good hands
    LOOSE_PASSIVE,   // Plays many hands, calls frequently  
    LOOSE_AGGRESSIVE // Plays many hands, bets/raises frequently
};

enum class PlayerAction {
    FOLD,
    CHECK,
    CALL,
    RAISE,
    ALL_IN
};

class Player {
private:
    std::string name;
    int chips;
    std::vector<Card> hand;
    std::vector<bool> cardsFaceUp; // Track which cards are face up (for stud games)
    int currentBet;
    int inFor; // Chips committed to pot during current betting round
    bool folded;
    bool allIn;
    PlayerPersonality personality;
    int playerId; // Stable identifier for hand history tracking
    mutable std::mt19937 rng; // For decision randomness

public:
    Player(const std::string& playerName, int startingChips, int id, 
           PlayerPersonality playerPersonality = PlayerPersonality::TIGHT_PASSIVE);
    
    // Getters
    const std::string& getName() const;
    int getChips() const;
    int getPlayerId() const;
    const std::vector<Card>& getHand() const;
    int getCurrentBet() const;
    int getInFor() const; // Get chips committed to pot this round
    bool hasFolded() const;
    bool isAllIn() const;
    int getHandSize() const;
    
    // Card management
    void addCard(const Card& card);
    void addCard(const Card& card, bool faceUp); // For stud games
    void clearHand();
    void showHand() const;
    void showStudHand() const; // Show stud-style hand (face up/down)
    std::vector<Card> getUpCards() const; // Get only face-up cards
    Card getLowestUpCard() const; // For bring-in determination
    
    // Chip management
    void addChips(int amount);
    void deductChips(int amount);
    bool canAfford(int amount) const;
    
    // Betting actions
    PlayerAction fold();
    PlayerAction check();
    PlayerAction call(int callAmount);
    PlayerAction raise(int raiseAmount);
    PlayerAction goAllIn();
    
    // Decision making - the main interface for AI players
    PlayerAction makeDecision(const HandHistory& history, int callAmount, bool canCheck = false) const;
    int calculateRaiseAmount(const HandHistory& history, int currentBet, const VariantInfo& variant) const;
    
    // Game state management
    void resetBet();
    void setBet(int amount);
    void addToInFor(int amount); // Add chips to inFor (deducted from stack)
    void resetInFor(); // Reset inFor to 0 at end of betting round
    void resetForNewHand();
    void showStatus(bool showCards = false) const;
    
private:
    // Decision helpers
    double evaluateHandStrength() const;
    double calculatePotOdds(int callAmount, int potSize) const;
    bool shouldFoldToAggression(const HandHistory& history, int callAmount) const;
    bool shouldBluff(const HandHistory& history) const;
    bool isHandPlayable() const; // Basic preflop hand selection
};

#endif 