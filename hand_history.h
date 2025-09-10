#ifndef HAND_HISTORY_H
#define HAND_HISTORY_H

#include "card.h"
#include "poker_variant.h"
#include <vector>
#include <string>

enum class ActionType {
    FOLD,
    CHECK, 
    CALL,
    RAISE,
    ALL_IN,
    POST_BLIND,    // For blind/ante posting
    DEAL_CARDS,    // Card dealing events
    REVEAL_BOARD   // Community card reveals
};

enum class HandHistoryRound {
    PRE_HAND,      // Setup, blinds, card dealing
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

struct PlayerInfo {
    int playerId;
    std::string name;
    int position;      // Seat number (0-based)
    int startingChips;
    bool isDealer;
};

struct GameAction {
    HandHistoryRound round;
    int playerId;           // -1 for non-player actions (dealing cards)
    ActionType actionType;
    int amount;             // Bet/raise amount, 0 for check/fold
    int potAfterAction;     // Total pot size after this action
    std::vector<Card> cardsDealt; // For DEAL_CARDS and REVEAL_BOARD actions
    std::string description; // Human readable description
};

struct HandResolution {
    std::vector<int> winners;
    std::vector<int> amounts;  // Amount each winner receives
    std::string description;   // e.g., "Alice wins with pair of aces"
};

class HandHistory {
private:
    PokerVariant variant;
    int handNumber;
    std::vector<PlayerInfo> players;
    std::vector<GameAction> actions;
    HandResolution resolution;
    bool isComplete;

public:
    HandHistory(PokerVariant gameVariant, int handNum);
    
    // Setup methods
    void addPlayer(int playerId, const std::string& name, int position, 
                   int chips, bool dealer = false);
    
    // Action recording methods
    void recordAction(HandHistoryRound round, int playerId, ActionType action, 
                     int amount, int potSize, const std::string& desc = "");
    void recordCardDeal(HandHistoryRound round, const std::vector<Card>& cards, 
                       const std::string& desc);
    void recordResolution(const std::vector<int>& winners, 
                         const std::vector<int>& amounts, const std::string& desc);
    
    // Query methods
    const std::vector<GameAction>& getActions() const;
    const std::vector<GameAction> getActionsForRound(HandHistoryRound round) const;
    const std::vector<GameAction> getActionsForPlayer(int playerId) const;
    const std::vector<PlayerInfo>& getPlayers() const;
    int getCurrentPot() const;
    int getLastRaiseAmount() const;
    bool hasPlayerActedThisRound(int playerId, HandHistoryRound round) const;
    
    // State queries
    bool isHandComplete() const;
    HandHistoryRound getCurrentRound() const;
    
    // Display methods  
    void printHistory() const;
    void printCurrentState() const;
    
private:
    // Helper methods
    std::string generateActionDescription(int playerId, ActionType action, int amount) const;
    std::string roundToString(HandHistoryRound round) const;
};

#endif