#ifndef POKER_GAME_H
#define POKER_GAME_H

#include "table.h"
#include "player.h"
#include "hand_evaluator.h"
#include "poker_variant.h"
#include <vector>

class PokerGame {
protected:
    Table* table;
    VariantConfig config;
    int currentPlayerIndex;
    bool handComplete;
    bool currentHandHasChoppedPot; // Track if current hand has chopped pot
    
public:
    PokerGame(Table* gameTable, const VariantConfig& gameConfig);
    virtual ~PokerGame() = default;
    
    // Pure virtual methods that each variant must implement
    virtual void startNewHand() = 0;
    virtual void dealInitialCards() = 0;
    virtual void runBettingRounds() = 0;
    virtual void conductShowdown() = 0;
    virtual bool isHandComplete() const = 0;
    
    // Virtual showdown methods (can be overridden for variant-specific behavior)
    virtual void awardPotsStaged(); // Award pots in reverse order (side pots first)
    virtual void awardPot(int potAmount, const std::vector<int>& eligiblePlayers); // Award single pot
    virtual void transferPotToWinners(int potAmount, const std::vector<int>& winners); // Variant-specific money transfer
    
    // Common methods
    virtual void showGameState() const;
    virtual std::vector<int> findWinners(const std::vector<int>& eligiblePlayers) const;
    
    // Common pot mechanics (same across all poker variants)
    virtual void collectBetsToInFor(); // Collect inFor amounts to pots at end of betting round
    virtual bool hasSidePots() const;
    virtual bool hasChoppedPots() const;
    virtual bool isInterestingHand() const;
    
    // Common betting actions (same logic across all variants)
    virtual bool playerCall(int playerIndex);
    virtual bool playerRaise(int playerIndex, int amount);
    virtual bool playerFold(int playerIndex);
    virtual bool playerAllIn(int playerIndex);
    virtual bool playerCheck(int playerIndex);
    
    // Utility functions for showdown (common operations)
    virtual std::vector<int> findBestHand(const std::vector<int>& eligiblePlayers) const; // Find winners among eligible players (virtual for variants)
    virtual void displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const; // Show hand descriptions (virtual for variants)
    void splitPotAmongWinners(int potAmount, const std::vector<int>& winners) const; // Calculate split amounts (doesn't transfer)
    
    // Getters
    VariantConfig getConfig() const { return config; }
    int getCurrentPlayerIndex() const { return currentPlayerIndex; }
};

#endif 