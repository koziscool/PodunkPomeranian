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
    
public:
    TexasHoldem(Table* gameTable, int smallBlind = 10, int bigBlind = 20);
    
    // Implement pure virtual methods
    void startNewHand() override;
    void dealInitialCards() override;
    void runBettingRounds() override;
    void conductShowdown() override;
    bool atShowdown() const override;
    
    // Hold'em specific methods
    void postBlinds();
    void nextPhase();
    BettingRound getCurrentRound() const;
};

#endif 