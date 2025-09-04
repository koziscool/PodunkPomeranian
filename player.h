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
    int currentBet;
    bool folded;
    bool allIn;

public:
    Player(const std::string& playerName, int startingChips);
    
    // Getters
    const std::string& getName() const;
    int getChips() const;
    const std::vector<Card>& getHand() const;
    int getCurrentBet() const;
    bool hasFolded() const;
    bool isAllIn() const;
    int getHandSize() const;
    
    // Card management
    void addCard(const Card& card);
    void clearHand();
    void showHand() const;
    
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
    void resetForNewHand();
    void showStatus(bool showCards = false) const;
};

#endif 