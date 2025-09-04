#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <vector>
#include <string>
#include <iomanip>

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

public:
    Player(const std::string& playerName, int startingChips);
    
    // Getters
    const std::string& getName() const;
    int getChips() const;
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
    
    // Game state management
    void resetBet();
    void setBet(int amount);
    void addToInFor(int amount); // Add chips to inFor (deducted from stack)
    void resetInFor(); // Reset inFor to 0 at end of betting round
    void resetForNewHand();
    void showStatus(bool showCards = false) const;
};

#endif 