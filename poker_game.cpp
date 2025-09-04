#include "poker_game.h"
#include <iostream>
#include <set>
#include <algorithm>

PokerGame::PokerGame(Table* gameTable, const VariantConfig& gameConfig)
    : table(gameTable), config(gameConfig), currentPlayerIndex(0), handComplete(false), currentHandHasChoppedPot(false) {}

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
    
    int callAmount = table->getCurrentBet() - player->getCurrentBet();
    if (callAmount <= 0) {
        return playerCheck(playerIndex);
    }
    
    int totalCallAmount = table->getCurrentBet();
    PlayerAction action = player->call(totalCallAmount);
    std::cout << player->getName() << " calls $" << totalCallAmount << std::endl;
    return action == PlayerAction::CALL;
}

bool PokerGame::playerRaise(int playerIndex, int amount) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    PlayerAction action = player->raise(amount);
    table->setCurrentBet(amount);
    std::cout << player->getName() << " raises to $" << amount << std::endl;
    return action == PlayerAction::RAISE;
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
    
    if (table->getCurrentBet() > 0) {
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