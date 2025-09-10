#ifndef TABLE_H
#define TABLE_H

#include "player.h"
#include "deck.h"
#include "side_pot.h"
#include <vector>
#include <memory>

class Table {
private:
    std::vector<std::unique_ptr<Player>> players;
    Deck deck;
    std::vector<Card> communityCards;
    int dealerPosition;
    int currentBet;
    SidePotManager sidePotManager;

public:
    Table();
    
    // Player management
    void addPlayer(const std::string& name, int chips, int playerId, PlayerPersonality personality = PlayerPersonality::TIGHT_PASSIVE);
    Player* getPlayer(int index);
    const Player* getPlayer(int index) const;
    int getPlayerCount() const;
    int getNextActivePlayer(int startIndex) const;
    
    // Game flow
    void startNewHand();
    Card dealCard();
    void dealHoleCards();
    void dealFlop();
    void dealTurn();
    void dealRiver();
    
    // Community cards
    const std::vector<Card>& getCommunityCards() const;
    void showCommunityCards() const;
    
    // Betting and pot management
    int getCurrentBet() const;
    void setCurrentBet(int bet);
    int getPot() const;
    void collectBets();
    void createSidePotsFromCurrentBets();
    void createSidePotsFromInFor(); // New method for inFor-based pot collection
    void returnUnmatchedChips(const std::vector<std::pair<int, int>>& playerBets);
    
    // Side pot management
    const SidePotManager& getSidePotManager() const;
    SidePotManager& getSidePotManager();
    void showPotBreakdown() const;
    
    // Display
    void showTable() const;
    
    // Dealer position
    int getDealerPosition() const;
    void advanceDealer();
    
    // Deck access
    Deck& getDeck();
};

#endif 