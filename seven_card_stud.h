#ifndef SEVEN_CARD_STUD_H
#define SEVEN_CARD_STUD_H

#include "poker_game.h"

enum class StudRound {
    THIRD_STREET,   // 2 down, 1 up
    FOURTH_STREET,  // 1 more up
    FIFTH_STREET,   // 1 more up
    SIXTH_STREET,   // 1 more up
    SEVENTH_STREET, // 1 more down
    SHOWDOWN
};

class SevenCardStud : public PokerGame {
private:
    StudRound currentRound;
    bool bettingComplete;
    std::vector<bool> hasActedThisRound;
    int bringInPlayer; // Player who must bring-in
    
public:
    SevenCardStud(Table* gameTable, int ante = 5, int bringIn = 10, int smallBet = 20, int largeBet = 40);
    
    // Implement pure virtual methods
    void startNewHand() override;
    void dealInitialCards() override;
    void runBettingRounds() override;
    void conductShowdown() override;
    bool isHandComplete() const override;
    
    // Stud specific methods
    void collectAntes();
    void dealThirdStreet();
    void dealFourthStreet();
    void dealFifthStreet();
    void dealSixthStreet();
    void dealSeventhStreet();
    void determineBringInPlayer();
    void startBettingRound(StudRound round);
    void nextStreet();
    void autoCompleteCurrentBettingRound();
    
    // Player actions
    bool playerComplete(int playerIndex); // Complete bring-in to small bet (7CS specific)
    bool playerBringIn(int playerIndex); // Bring-in (7CS specific)
    
    // 7CS-specific wrappers that handle hasActedThisRound tracking
    bool studPlayerCall(int playerIndex);
    bool studPlayerCheck(int playerIndex);
    bool studPlayerFold(int playerIndex);
    
    // Game state
    bool canPlayerAct(int playerIndex) const;
    StudRound getCurrentRound() const;
    int getCurrentBet() const;
    
    // Betting logic
    void advanceToNextPlayer();
    bool isBettingComplete() const;
    bool allPlayersActed() const;
    bool allPlayersActedThisRound() const;
    int countActivePlayers() const;
    bool allRemainingPlayersAllIn() const;
    bool isActionComplete() const;
    
    // Helper methods (7CS specific)
    int getNextActivePlayer(int startIndex) const;
    void resetBettingState();
    void showStudGameState() const;
    int findHighestShowingHand() const; // Find player with highest showing hand for betting order
    
    // Override base class methods for 7CS specifics
    std::vector<int> findBestHand(const std::vector<int>& eligiblePlayers) const override; // No community cards in 7CS
    void displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const override; // No community cards in 7CS
};

#endif 