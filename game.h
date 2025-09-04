#ifndef GAME_H
#define GAME_H

#include "table.h"
#include "player.h"
#include "hand_evaluator.h"
#include <vector>

enum class BettingRound {
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

class Game {
private:
    Table* table;
    BettingRound currentRound;
    int currentPlayerIndex;
    bool bettingComplete;
    std::vector<bool> hasActedThisRound;
    int smallBlindAmount;
    int bigBlindAmount;
    bool currentHandHasChoppedPot; // Track if current hand has chopped pots
    
public:
    Game(Table* gameTable, int smallBlind = 10, int bigBlind = 20);
    
    // Game flow
    void startNewHand();
    void postBlinds();
    void startBettingRound(BettingRound round);
    void nextPhase();
    bool isHandComplete() const;
    void autoCompleteCurrentBettingRound();  // Auto-check to complete current betting round
    void forceCompleteBettingRound();        // Force current betting round to be marked complete
    
    // Player actions
    bool playerFold(int playerIndex);
    bool playerCheck(int playerIndex);
    bool playerCall(int playerIndex);
    bool playerRaise(int playerIndex, int amount);
    bool playerAllIn(int playerIndex);
    
    // Game state
    bool canPlayerAct(int playerIndex) const;
    bool isValidAction(int playerIndex, PlayerAction action, int amount = 0) const;
    int getCurrentPlayerIndex() const;
    BettingRound getCurrentRound() const;
    
    // Betting logic
    void advanceToNextPlayer();
    bool isBettingComplete() const;
    int getMinimumRaise() const;
    bool allPlayersActed() const;
    bool allPlayersActedThisRound() const;
    int countActivePlayers() const;
    bool allRemainingPlayersAllIn() const;
    bool isActionComplete() const;
    bool allOtherPlayersAllIn(int excludePlayerIndex) const;
    
    // Hand resolution
    void conductShowdown();
    void awardPotsStaged();
    void awardSinglePot(int potAmount, const std::vector<int>& winners);
    
    // Detection methods for interesting hands
    bool hasSidePots() const;
    bool hasChoppedPots() const;
    bool isInterestingHand() const;
    
    // Display
    void showGameState() const;
    
private:
    // Helper methods
    int getNextActivePlayer(int startIndex) const;
    void resetBettingState();
    std::vector<int> findWinners(const std::vector<int>& eligiblePlayers) const;
};

#endif 