#include "hand_history.h"
#include <iostream>
#include <algorithm>

HandHistory::HandHistory(PokerVariant gameVariant, int handNum) 
    : variant(gameVariant), handNumber(handNum), isComplete(false) {
}

void HandHistory::addPlayer(int playerId, const std::string& name, int position, 
                           int chips, bool dealer) {
    PlayerInfo player = {playerId, name, position, chips, dealer};
    players.push_back(player);
}

void HandHistory::recordAction(HandHistoryRound round, int playerId, ActionType action, 
                              int amount, int potSize, const std::string& desc) {
    GameAction gameAction;
    gameAction.round = round;
    gameAction.playerId = playerId;
    gameAction.actionType = action;
    gameAction.amount = amount;
    gameAction.potAfterAction = potSize;
    gameAction.description = desc.empty() ? generateActionDescription(playerId, action, amount) : desc;
    
    actions.push_back(gameAction);
}

void HandHistory::recordCardDeal(HandHistoryRound round, const std::vector<Card>& cards, 
                                const std::string& desc) {
    GameAction gameAction;
    gameAction.round = round;
    gameAction.playerId = -1; // No specific player
    gameAction.actionType = (round == HandHistoryRound::PRE_HAND) ? ActionType::DEAL_CARDS : ActionType::REVEAL_BOARD;
    gameAction.amount = 0;
    gameAction.potAfterAction = getCurrentPot();
    gameAction.cardsDealt = cards;
    gameAction.description = desc;
    
    actions.push_back(gameAction);
}

void HandHistory::recordResolution(const std::vector<int>& winners, 
                                  const std::vector<int>& amounts, const std::string& desc) {
    resolution.winners = winners;
    resolution.amounts = amounts;
    resolution.description = desc;
    isComplete = true;
}

const std::vector<GameAction>& HandHistory::getActions() const {
    return actions;
}

const std::vector<GameAction> HandHistory::getActionsForRound(HandHistoryRound round) const {
    std::vector<GameAction> roundActions;
    for (const auto& action : actions) {
        if (action.round == round) {
            roundActions.push_back(action);
        }
    }
    return roundActions;
}

const std::vector<GameAction> HandHistory::getActionsForPlayer(int playerId) const {
    std::vector<GameAction> playerActions;
    for (const auto& action : actions) {
        if (action.playerId == playerId) {
            playerActions.push_back(action);
        }
    }
    return playerActions;
}

const std::vector<PlayerInfo>& HandHistory::getPlayers() const {
    return players;
}

int HandHistory::getCurrentPot() const {
    if (actions.empty()) return 0;
    return actions.back().potAfterAction;
}

int HandHistory::getLastRaiseAmount() const {
    // Look backwards through actions to find the last raise
    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        if (it->actionType == ActionType::RAISE) {
            return it->amount;
        }
    }
    return 0; // No raises found
}

bool HandHistory::hasPlayerActedThisRound(int playerId, HandHistoryRound round) const {
    for (const auto& action : actions) {
        if (action.round == round && action.playerId == playerId && 
            action.actionType != ActionType::POST_BLIND) {
            return true;
        }
    }
    return false;
}

bool HandHistory::isHandComplete() const {
    return isComplete;
}

HandHistoryRound HandHistory::getCurrentRound() const {
    if (actions.empty()) return HandHistoryRound::PRE_HAND;
    return actions.back().round;
}

void HandHistory::printHistory() const {
    std::cout << "=== HAND " << handNumber << " HISTORY ===" << std::endl;
    
    // Print players
    std::cout << "Players: ";
    for (const auto& player : players) {
        std::cout << player.name << " (ID:" << player.playerId << ", $" << player.startingChips << ")";
        if (player.isDealer) std::cout << " [D]";
        std::cout << " ";
    }
    std::cout << std::endl << std::endl;
    
    // Print actions by round
    HandHistoryRound currentPrintRound = HandHistoryRound::PRE_HAND;
    for (const auto& action : actions) {
        if (action.round != currentPrintRound) {
            currentPrintRound = action.round;
            std::cout << "\n--- " << roundToString(currentPrintRound) << " ---" << std::endl;
        }
        
        if (action.playerId >= 0) {
            // Find player name
            std::string playerName = "Unknown";
            for (const auto& player : players) {
                if (player.playerId == action.playerId) {
                    playerName = player.name;
                    break;
                }
            }
            std::cout << playerName << ": ";
        }
        
        std::cout << action.description;
        if (action.potAfterAction > 0) {
            std::cout << " (Pot: $" << action.potAfterAction << ")";
        }
        std::cout << std::endl;
    }
    
    // Print resolution
    if (isComplete) {
        std::cout << "\n--- RESOLUTION ---" << std::endl;
        std::cout << resolution.description << std::endl;
    }
}

void HandHistory::printCurrentState() const {
    std::cout << "Current pot: $" << getCurrentPot() << std::endl;
    std::cout << "Current round: " << roundToString(getCurrentRound()) << std::endl;
}

std::string HandHistory::generateActionDescription(int playerId, ActionType action, int amount) const {
    std::string playerName = "Unknown";
    for (const auto& player : players) {
        if (player.playerId == playerId) {
            playerName = player.name;
            break;
        }
    }
    
    switch (action) {
        case ActionType::FOLD:
            return "folds";
        case ActionType::CHECK:
            return "checks";
        case ActionType::CALL:
            return "calls $" + std::to_string(amount);
        case ActionType::RAISE:
            return "raises to $" + std::to_string(amount);
        case ActionType::ALL_IN:
            return "goes all-in for $" + std::to_string(amount);
        case ActionType::POST_BLIND:
            return "posts $" + std::to_string(amount);
        default:
            return "acts";
    }
}

std::string HandHistory::roundToString(HandHistoryRound round) const {
    switch (round) {
        case HandHistoryRound::PRE_HAND: return "PRE-HAND";
        case HandHistoryRound::PRE_FLOP: return "PRE-FLOP";
        case HandHistoryRound::FLOP: return "FLOP";
        case HandHistoryRound::TURN: return "TURN";
        case HandHistoryRound::RIVER: return "RIVER";
        case HandHistoryRound::SHOWDOWN: return "SHOWDOWN";
        default: return "UNKNOWN";
    }
}