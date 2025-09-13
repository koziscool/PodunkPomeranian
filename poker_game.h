#ifndef POKER_GAME_H
#define POKER_GAME_H

#include "table.h"
#include "player.h"
#include "hand_evaluator.h"
#include "poker_variant.h"
#include "variants.h"
#include "hand_history.h"
#include <vector>

class PokerGame {
protected:
    Table* table;
    VariantInfo variantInfo;
    int currentPlayerIndex;
    bool handComplete;
    bool currentHandHasChoppedPot; // Track if current hand has chopped pot
    HandHistory handHistory;
    std::vector<bool> hasActedThisRound;
    UnifiedBettingRound currentRound;
    int betCount; // Track number of bets in current round for limit games
    int currentActionPotIndex; // Index of the pot that receives new money (0=main, 1=side1, etc.)
    
    // Hi-Lo specific winner tracking
    std::vector<int> hiWinners;
    std::vector<int> loWinners;
    
public:
    PokerGame(Table* gameTable, const VariantInfo& variant);
    virtual ~PokerGame() = default;
    
    // Unified game flow methods (no longer pure virtual)
    virtual void startNewHand();
    virtual void dealInitialCards();
    virtual void runBettingRounds();
    virtual void conductShowdown();
    virtual void awardPotsWithoutShowdown();
    virtual bool atShowdown() const;
    
    // Structure-specific game flow methods
    virtual void gameFlowForBOARD();
    virtual void gameFlowForSTUD();
    
    // Unified betting operations
    virtual void postBlinds();        // For BOARD games
    virtual void postAntesAndBringIn(); // For STUD games
    virtual void nextRound();         // Advance to next unified round
    
    // Generic hand completion logic (same for all poker variants)
    virtual bool isHandComplete() const;
    
    // Virtual showdown methods (can be overridden for variant-specific behavior)
    virtual void awardPotsStaged(); // Award pots in reverse order (side pots first)
    virtual void awardPot(int potAmount, const std::vector<int>& eligiblePlayers); // Award single pot
    virtual void transferPotToWinners(int potAmount, const std::vector<int>& winners); // Variant-specific money transfer
    
    // Common methods
    virtual void showGameState() const;
    virtual std::vector<int> findWinners(const std::vector<int>& eligiblePlayers);
    
    // Common pot mechanics (same across all poker variants)
    virtual void collectBetsToInFor(); // Collect inFor amounts to pots at end of betting round
    virtual void handleAllInSidePots(int allInPlayerIndex); // Handle side pot creation when player goes all-in
    virtual bool hasSidePots() const;
    virtual bool hasChoppedPots() const;
    virtual bool isInterestingHand() const;
    
    // Common betting actions (same logic across all variants)
    virtual bool playerCall(int playerIndex);
    virtual bool playerRaise(int playerIndex, int amount);
    virtual bool playerFold(int playerIndex);
    virtual bool playerAllIn(int playerIndex);
    virtual bool playerCheck(int playerIndex);
    
    // Common betting round management
    virtual void initializeHandHistory(int handNumber);
    virtual void recordPlayerAction(HandHistoryRound round, int playerId, ActionType actionType, int amount, const std::string& description);
    virtual bool isBettingComplete() const;
    virtual void advanceToNextPlayer();
    virtual int countActivePlayers() const;
    virtual bool allRemainingPlayersAllIn() const;
    virtual bool canPlayerAct(int playerIndex) const;
    virtual void resetBettingRound();
    
    // Intelligent betting round completion using player AI
    virtual void completeBettingRound(HandHistoryRound historyRound);
    
    // Utility functions for showdown (common operations)
    virtual std::vector<int> findBestHand(const std::vector<int>& eligiblePlayers); // Find winners among eligible players (virtual for variants)
    virtual void displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const; // Show hand descriptions (virtual for variants)
    void splitPotAmongWinners(int potAmount, const std::vector<int>& winners) const; // Calculate split amounts (doesn't transfer)
    
    // Variant-specific hand evaluation methods
    HandResult evaluateOmahaHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const;
    LowHandResult evaluateOmahaLowHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const;
    std::vector<int> findHiLoWinners(const std::vector<int>& eligiblePlayers);
    void displayHiLoWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const;
    void transferHiLoPotsToWinners(int potAmount);
    
    // Stud-specific betting order methods
    int findStudFirstToAct() const; // Find player who acts first based on up cards
    bool determineBettorForStud(const std::vector<Card>& hand1, const std::vector<Card>& hand2) const; // Compare Stud up cards for betting order
    
    // Getters
    VariantInfo getVariantInfo() const { return variantInfo; }
    int getCurrentPlayerIndex() const { return currentPlayerIndex; }
};

#endif 