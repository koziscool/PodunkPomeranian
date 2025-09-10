#include "poker_game.h"
#include <iostream>
#include <set>
#include <algorithm>

PokerGame::PokerGame(Table* gameTable, const VariantConfig& gameConfig)
    : table(gameTable), config(gameConfig), currentPlayerIndex(0), handComplete(false), 
      currentHandHasChoppedPot(false), handHistory(config.variant, 1) {}

void PokerGame::showGameState() const {
    table->showTable();
}

std::vector<int> PokerGame::findWinners(const std::vector<int>& eligiblePlayers) const {
    // Delegate to the new findBestHand method
    return findBestHand(eligiblePlayers);
}

// Common pot mechanics (same across all poker variants)
void PokerGame::collectBetsToInFor() {
    table->collectBets();
}

bool PokerGame::hasSidePots() const {
    return table->getSidePotManager().getPots().size() > 1;
}

bool PokerGame::hasChoppedPots() const {
    return currentHandHasChoppedPot;
}

bool PokerGame::isInterestingHand() const {
    return hasSidePots() || hasChoppedPots();
}

// Common betting actions (same logic across all variants)
bool PokerGame::playerCall(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    int currentBet = table->getCurrentBet();
    int callAmount = currentBet - player->getInFor();
    if (callAmount <= 0) {
        return playerCheck(playerIndex);
    }
    
    if (callAmount >= player->getChips()) {
        return playerAllIn(playerIndex);
    }
    
    player->addToInFor(callAmount);
    std::cout << player->getName() << " calls $" << currentBet << std::endl;
    return true;
}

bool PokerGame::playerRaise(int playerIndex, int amount) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    // Calculate additional amount needed based on inFor system
    int additionalAmount = amount - player->getInFor();
    if (additionalAmount <= 0) {
        // Invalid raise - they're already at or above the raise amount
        return false;
    }
    
    if (additionalAmount >= player->getChips()) {
        return playerAllIn(playerIndex);
    }
    
    player->addToInFor(additionalAmount);
    int previousBet = table->getCurrentBet();
    table->setCurrentBet(amount);
    
    if (previousBet == 0) {
        std::cout << player->getName() << " bets $" << amount << std::endl;
    } else {
        std::cout << player->getName() << " raises to $" << amount << std::endl;
    }
    return true;
}

bool PokerGame::playerFold(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    PlayerAction action = player->fold();
    std::cout << player->getName() << " folds" << std::endl;
    return action == PlayerAction::FOLD;
}

bool PokerGame::playerAllIn(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    int allInAmount = player->getCurrentBet() + player->getChips();
    PlayerAction action = player->goAllIn();
    table->setCurrentBet(allInAmount);
    std::cout << player->getName() << " goes all-in for $" << allInAmount << std::endl;
    return action == PlayerAction::ALL_IN;
}

bool PokerGame::playerCheck(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    int currentBet = table->getCurrentBet();
    int callAmount = currentBet - player->getInFor();
    
    if (callAmount > 0) {
        std::cout << player->getName() << " cannot check - must call or fold" << std::endl;
        return false;
    }
    
    PlayerAction action = player->check();
    std::cout << player->getName() << " checks" << std::endl;
    return action == PlayerAction::CHECK;
}

// Virtual showdown methods (default implementations)
void PokerGame::awardPotsStaged() {
    const auto& pots = table->getSidePotManager().getPots();
    
    // Award pots in reverse order (side pots first, main pot last)
    for (int i = static_cast<int>(pots.size()) - 1; i >= 0; i--) {
        const SidePot& pot = pots[i];
        std::vector<int> eligiblePlayers(pot.eligiblePlayers.begin(), pot.eligiblePlayers.end());
        
        if (!eligiblePlayers.empty()) {
            std::cout << "\n=== AWARDING ";
            if (i == 0) {
                std::cout << "MAIN POT ($" << pot.amount << ") ===";
            } else {
                std::cout << "SIDE POT " << i << " ($" << pot.amount << ") ===";
            }
            std::cout << std::endl;
            
            awardPot(pot.amount, eligiblePlayers);
        }
    }
}

void PokerGame::awardPot(int potAmount, const std::vector<int>& eligiblePlayers) {
    if (eligiblePlayers.empty()) {
        std::cout << "No eligible players for pot!" << std::endl;
        return;
    }
    
    std::vector<int> winners = findBestHand(eligiblePlayers);
    displayWinningHands(winners, eligiblePlayers);
    transferPotToWinners(potAmount, winners);
}

void PokerGame::transferPotToWinners(int potAmount, const std::vector<int>& winners) {
    // Default implementation for standard high-only games (Hold'em, 7-Card Stud)
    if (winners.empty()) {
        std::cout << "No winners found for pot!" << std::endl;
        return;
    }
    
    // Track if this is a chopped pot
    if (winners.size() > 1) {
        currentHandHasChoppedPot = true;
    }
    
    int amountPerWinner = potAmount / static_cast<int>(winners.size());
    int remainder = potAmount % static_cast<int>(winners.size());
    
    for (size_t i = 0; i < winners.size(); i++) {
        Player* winner = table->getPlayer(winners[i]);
        if (winner) {
            int award = amountPerWinner + (i < static_cast<size_t>(remainder) ? 1 : 0);
            winner->addChips(award);
            std::cout << winner->getName() << " wins $" << award << std::endl;
        }
    }
}

// Utility functions for showdown
std::vector<int> PokerGame::findBestHand(const std::vector<int>& eligiblePlayers) const {
    if (eligiblePlayers.empty()) {
        return {};
    }
    
    HandResult bestHand;
    bestHand.rank = HandRank::HIGH_CARD;
    std::vector<int> winners;
    
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult hand = HandEvaluator::evaluateHand(
                player->getHand(), 
                table->getCommunityCards()
            );
            
            if (hand > bestHand) {
                bestHand = hand;
                winners.clear();
                winners.push_back(playerIndex);
            } else if (hand == bestHand) {
                winners.push_back(playerIndex);
            }
        }
    }
    
    return winners;
}

void PokerGame::displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const {
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult hand = HandEvaluator::evaluateHand(
                player->getHand(), 
                table->getCommunityCards()
            );
            
            bool isWinner = std::find(winners.begin(), winners.end(), playerIndex) != winners.end();
            std::cout << player->getName() << ": " << hand.description;
            if (isWinner) {
                std::cout << " (WINNER)";
            }
            std::cout << std::endl;
        }
    }
}

void PokerGame::splitPotAmongWinners(int potAmount, const std::vector<int>& winners) const {
    // Utility function to calculate split amounts without transferring chips
    // This can be used by variants that need to know split amounts before transferring
    if (winners.empty()) return;
    
    int amountPerWinner = potAmount / static_cast<int>(winners.size());
    int remainder = potAmount % static_cast<int>(winners.size());
    
    std::cout << "Pot split: $" << amountPerWinner << " each";
    if (remainder > 0) {
        std::cout << " (+" << remainder << " to first " << remainder << " winner" << (remainder > 1 ? "s" : "") << ")";
    }
    std::cout << std::endl;
}

// Common betting round management methods
void PokerGame::initializeHandHistory(int handNumber) {
    handHistory = HandHistory(config.variant, handNumber);
    hasActedThisRound.assign(table->getPlayerCount(), false);
    
    // Add all players to hand history
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            bool isDealer = (i == table->getDealerPosition());
            handHistory.addPlayer(player->getPlayerId(), player->getName(), i, player->getChips(), isDealer);
        }
    }
}

void PokerGame::recordPlayerAction(HandHistoryRound round, int playerId, ActionType actionType, int amount, const std::string& description) {
    handHistory.recordAction(round, playerId, actionType, amount, table->getCurrentBet(), description);
}

bool PokerGame::isBettingComplete() const {
    int currentBet = table->getCurrentBet();
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            // If player hasn't acted this round, betting isn't complete
            if (!hasActedThisRound[i]) {
                std::cout << "DEBUG: Player " << i << " (" << player->getName() << ") hasn't acted this round" << std::endl;
                return false;
            }
            // If player's inFor amount doesn't match current bet, betting isn't complete
            if (player->getInFor() < currentBet) {
                std::cout << "DEBUG: Player " << i << " (" << player->getName() << ") inFor=" << player->getInFor() << " < currentBet=" << currentBet << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

void PokerGame::advanceToNextPlayer() {
    int playerCount = table->getPlayerCount();
    int startIndex = currentPlayerIndex;
    
    do {
        currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
        Player* player = table->getPlayer(currentPlayerIndex);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            return; // Found next active player
        }
    } while (currentPlayerIndex != startIndex);
    
    // No active players found
    currentPlayerIndex = -1;
}

int PokerGame::countActivePlayers() const {
    int count = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            count++;
        }
    }
    return count;
}

bool PokerGame::allRemainingPlayersAllIn() const {
    int activeNonAllInCount = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            activeNonAllInCount++;
        }
    }
    return activeNonAllInCount <= 1;
}

bool PokerGame::canPlayerAct(int playerIndex) const {
    Player* player = table->getPlayer(playerIndex);
    return player && !player->hasFolded() && !player->isAllIn() && playerIndex == currentPlayerIndex;
}

void PokerGame::resetBettingRound() {
    hasActedThisRound.assign(table->getPlayerCount(), false);
}

void PokerGame::completeBettingRound(HandHistoryRound currentRound) {
    int actionCount = 0;
    const int MAX_ACTIONS = 10; // Reduced safety limit
    
    // Quick exit if only one player remains
    if (countActivePlayers() <= 1) {
        return;
    }
    
    while (!isBettingComplete() && actionCount < MAX_ACTIONS && countActivePlayers() > 1) {
        if (actionCount > 5) {
            std::cout << "DEBUG: actionCount=" << actionCount << ", isBettingComplete=" << isBettingComplete() << ", activePlayers=" << countActivePlayers() << std::endl;
        }
        int playerIndex = currentPlayerIndex;
        
        if (playerIndex == -1 || !canPlayerAct(playerIndex)) {
            break;
        }
        
        Player* player = table->getPlayer(playerIndex);
        if (!player) {
            break;
        }
        
        int currentBet = table->getCurrentBet();
        int callAmount = currentBet - player->getInFor();
        bool canCheck = (callAmount <= 0);
        
        PlayerAction decision = player->makeDecision(handHistory, callAmount, canCheck);
        
        // Execute the player's decision
        std::string actionDesc;
        switch (decision) {
            case PlayerAction::FOLD:
                playerFold(playerIndex);
                actionDesc = "folds";
                break;
            case PlayerAction::CHECK:
                playerCheck(playerIndex);
                actionDesc = "checks";
                break;
            case PlayerAction::CALL:
                playerCall(playerIndex);
                actionDesc = "calls $" + std::to_string(callAmount);
                break;
            case PlayerAction::RAISE: {
                int raiseAmount = player->calculateRaiseAmount(handHistory, currentBet);
                playerRaise(playerIndex, raiseAmount);
                if (currentBet == 0) {
                    actionDesc = "bets $" + std::to_string(raiseAmount);
                } else {
                    actionDesc = "raises to $" + std::to_string(raiseAmount);
                }
                break;
            }
            case PlayerAction::ALL_IN:
                playerAllIn(playerIndex);
                actionDesc = "goes all-in for $" + std::to_string(player->getChips());
                break;
        }
        
        // Record the action in hand history
        recordPlayerAction(currentRound, player->getPlayerId(), 
                          static_cast<ActionType>(decision), 
                          (decision == PlayerAction::RAISE) ? player->calculateRaiseAmount(handHistory, currentBet) : callAmount,
                          actionDesc);
        
        // Mark player as having acted
        hasActedThisRound[playerIndex] = true;
        
        // Advance to next player
        advanceToNextPlayer();
        actionCount++;
        
        // Emergency break: if everyone is just checking repeatedly, force round to end
        if (actionCount >= 6 && decision == PlayerAction::CHECK) {
            // If we've had 6+ consecutive checks, betting round should be over
            break;
        }
    }
    
    // Collect the inFor amounts to pots
    collectBetsToInFor();
}

// Generic hand completion logic (same for all poker variants)
bool PokerGame::isHandComplete() const {
    int activePlayers = countActivePlayers();
    return atShowdown() || activePlayers <= 1;
} 