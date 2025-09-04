#ifndef TEXAS_HOLDEM_H
#define TEXAS_HOLDEM_H

#include "poker_game.h"

enum class BettingRound {
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

class TexasHoldem : public PokerGame {
private:
    BettingRound currentRound;
    bool bettingComplete;
    std::vector<bool> hasActedThisRound;
    
public:
    TexasHoldem(Table* gameTable, int smallBlind = 10, int bigBlind = 20);
    
    // Implement pure virtual methods
    void startNewHand() override;
    void dealInitialCards() override;
    void runBettingRounds() override;
    void conductShowdown() override;
    bool isHandComplete() const override;
    
    // Hold'em specific methods
    void postBlinds();
    void startBettingRound(BettingRound round);
    void nextPhase();
    void autoCompleteCurrentBettingRound();
    
    // Player actions (Hold'em specific - placeholder for future implementation)
    // Basic actions inherited from base class: playerCall, playerCheck, playerFold, playerRaise, playerAllIn
    
    // Game state
    bool canPlayerAct(int playerIndex) const;
    BettingRound getCurrentRound() const;
    
    // Betting logic
    void advanceToNextPlayer();
    bool isBettingComplete() const;
    bool allPlayersActed() const;
    bool allPlayersActedThisRound() const;
    int countActivePlayers() const;
    bool allRemainingPlayersAllIn() const;
    bool isActionComplete() const;
    
    // Helper methods
    int getNextActivePlayer(int startIndex) const;
    void resetBettingState();
};

#endif 