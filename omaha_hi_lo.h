#ifndef OMAHA_HI_LO_H
#define OMAHA_HI_LO_H

#include "poker_game.h"
#include "poker_variant.h"

enum class OmahaBettingRound {
    PRE_FLOP,
    FLOP, 
    TURN,
    RIVER,
    SHOWDOWN
};

class OmahaHiLo : public PokerGame {
private:
    VariantConfig config;
    OmahaBettingRound currentRound;
    bool bettingComplete;
    std::vector<bool> hasActedThisRound;
    int currentPlayerIndex;

public:
    OmahaHiLo(Table* gameTable);
    
    // Core game flow
    void startNewHand() override;
    void dealInitialCards() override;
    void runBettingRounds() override;
    void conductShowdown() override;
    bool isHandComplete() const override;
    
    // Omaha-specific methods
    void postBlinds();
    void startBettingRound(OmahaBettingRound round);
    void nextPhase();
    void autoCompleteCurrentBettingRound();
    void showOmahaGameState();
    
    // Betting logic (same as Hold'em)
    bool canPlayerAct(int playerIndex) const;
    bool isBettingComplete() const;
    int countActivePlayers() const;
    bool allRemainingPlayersAllIn() const;
    bool isActionComplete() const;
    void advanceToNextPlayer();
    
    // Showdown methods (override for Hi-Lo split)
    void transferPotToWinners(int potAmount, const std::vector<int>& winners) override;
    std::vector<int> findBestHand(const std::vector<int>& eligiblePlayers) const override; // Override for Omaha 2+3 rule
    void displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const override;
    
    // Hi-Lo specific evaluation
    std::vector<int> findHighWinners(const std::vector<int>& eligiblePlayers) const;
    std::vector<int> findLowWinners(const std::vector<int>& eligiblePlayers) const;
    bool qualifiesForLow(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const;
    
    // Omaha-specific hand evaluation (2+3 rule)
    HandResult evaluateOmahaHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const;
    HandResult evaluateOmahaLowHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const;
    bool hasQualifyingLow(const std::vector<Card>& fiveCards) const;
    HandResult evaluateLowHand(const std::vector<Card>& fiveCards) const;
};

#endif 