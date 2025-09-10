#include "table.h"
#include <iostream>
#include <iomanip>
#include <set> // Added for std::set

Table::Table() : dealerPosition(0), currentBet(0) {
    deck.shuffle();
}

void Table::addPlayer(const std::string& name, int chips, int playerId, PlayerPersonality personality) {
    players.push_back(std::make_unique<Player>(name, chips, playerId, personality));
}

Player* Table::getPlayer(int index) {
    if (index >= 0 && index < static_cast<int>(players.size())) {
        return players[index].get();
    }
    return nullptr;
}

const Player* Table::getPlayer(int index) const {
    if (index >= 0 && index < static_cast<int>(players.size())) {
        return players[index].get();
    }
    return nullptr;
}

int Table::getPlayerCount() const {
    return static_cast<int>(players.size());
}

int Table::getNextActivePlayer(int startIndex) const {
    int playerCount = getPlayerCount();
    if (playerCount == 0) return -1;
    
    for (int i = 1; i <= playerCount; i++) {
        int nextIndex = (startIndex + i) % playerCount;
        Player* player = players[nextIndex].get();
        if (player && !player->hasFolded() && !player->isAllIn()) {
            return nextIndex;
        }
    }
    return -1;
}

void Table::startNewHand() {
    // Reset all players for new hand
    for (auto& player : players) {
        player->resetForNewHand();
    }
    
    // Reset table state
    communityCards.clear();
    currentBet = 0;
    sidePotManager.clearPots();
    
    // Shuffle deck
    deck.reset();
    deck.shuffle();
    
    // Advance dealer
    advanceDealer();
    
    std::cout << "\n=== NEW HAND - Dealer: " << players[dealerPosition]->getName() << " ===" << std::endl;
}

Card Table::dealCard() {
    return deck.dealCard();
}

void Table::dealHoleCards() {
    // Deal 2 cards to each player
    for (int round = 0; round < 2; round++) {
        for (auto& player : players) {
            if (!player->hasFolded()) {
                player->addCard(dealCard());
            }
        }
    }
}

void Table::dealFlop() {

    // Burn card
    dealCard();
    
    // Deal 3 community cards
    for (int i = 0; i < 3; i++) {
        communityCards.push_back(dealCard());
    }
    
    // Show the board
    showCommunityCards();
}

void Table::dealTurn() {

    // Burn card
    dealCard();
    
    // Deal 1 community card
    communityCards.push_back(dealCard());
    
    // Show the board
    showCommunityCards();
}

void Table::dealRiver() {

    // Burn card
    dealCard();
    
    // Deal 1 community card
    communityCards.push_back(dealCard());
    
    // Show the board
    showCommunityCards();
}

const std::vector<Card>& Table::getCommunityCards() const {
    return communityCards;
}

void Table::showCommunityCards() const {
    int totalPot = getPot();
    int mainPot = sidePotManager.getMainPotAmount();
    
    if (communityCards.empty()) {
        std::cout << "Community Cards: (none) | Total Pot: $" << totalPot;
        std::cout << " | Main Pot: $" << mainPot << std::endl;
        return;
    }
    
    std::cout << "Community Cards: ";
    for (const auto& card : communityCards) {
        std::cout << card.toString() << " ";
    }
    std::cout << "| Total Pot: $" << totalPot;
    std::cout << " | Main Pot: $" << mainPot;
    
    // Show side pots if any exist
    // We need to check if there are multiple pots by getting pot count from sidePotManager
    // For now, let's check if total pot is greater than main pot
    if (totalPot > mainPot) {
        int sidePotAmount = totalPot - mainPot;
        std::cout << "   Side Pot 1: $" << sidePotAmount;
    }
    
    std::cout << std::endl;
}

int Table::getCurrentBet() const {
    return currentBet;
}

void Table::setCurrentBet(int bet) {
    currentBet = bet;
}

int Table::getPot() const {
    return sidePotManager.getTotalPotAmount();
}

void Table::collectBets() {
    // Only call createSidePotsFromCurrentBets if there are actually current bets to collect
    bool hasCurrentBets = false;
    for (const auto& player : players) {
        if (player && player->getInFor() > 0) {
            hasCurrentBets = true;
            break;
        }
    }
    
    if (hasCurrentBets) {
        createSidePotsFromInFor();
    }
    
    for (auto& player : players) {
        player->resetBet();
        player->resetInFor();
    }
    currentBet = 0;
}

void Table::createSidePotsFromCurrentBets() {
    std::vector<std::pair<int, int>> playerBets;
    int foldedPlayerMoney = 0;
    
    // Separate active players from folded players
    for (int i = 0; i < getPlayerCount(); i++) {
        Player* player = getPlayer(i);
        if (player && player->getCurrentBet() > 0) {
            if (player->hasFolded()) {
                foldedPlayerMoney += player->getCurrentBet();
            } else {
                playerBets.emplace_back(i, player->getCurrentBet());
            }
        }
    }
    
    // Create side pots from active players
    sidePotManager.createSidePotsFromBets(playerBets);
    
    // Add folded player money to the main pot
    if (foldedPlayerMoney > 0) {
        sidePotManager.addToMainPot(foldedPlayerMoney);
    }
    
    // Return unmatched chips to players
    returnUnmatchedChips(playerBets);
}

void Table::createSidePotsFromInFor() {
    std::vector<std::pair<int, int>> playerBets;
    int foldedPlayerMoney = 0;
    
    // Separate active players from folded players
    for (int i = 0; i < getPlayerCount(); i++) {
        Player* player = getPlayer(i);
        if (player && player->getInFor() > 0) {
            if (player->hasFolded()) {
                foldedPlayerMoney += player->getInFor();
            } else {
                playerBets.emplace_back(i, player->getInFor());
            }
        }
    }
    
    // Check if all active players bet the same amount (no side pots needed)
    bool allSameAmount = true;
    if (!playerBets.empty()) {
        int firstAmount = playerBets[0].second;
        for (const auto& bet : playerBets) {
            if (bet.second != firstAmount) {
                allSameAmount = false;
                break;
            }
        }
    }
    
    if (allSameAmount && !playerBets.empty()) {
        // No side pots needed - just add all amounts to main pot
        int totalBettingRoundAmount = 0;
        std::set<int> eligiblePlayers;
        
        for (const auto& bet : playerBets) {
            totalBettingRoundAmount += bet.second;
            eligiblePlayers.insert(bet.first); // Add player to eligible list
        }
        totalBettingRoundAmount += foldedPlayerMoney;
        
        // If there's already a main pot, add to it and update eligible players
        if (!sidePotManager.getPots().empty()) {
            sidePotManager.addToMainPot(totalBettingRoundAmount);
            // Update eligible players for the main pot
            sidePotManager.addEligiblePlayersToMainPot(eligiblePlayers);
        } else {
            // Create new main pot with eligible players
            sidePotManager.addToPot(totalBettingRoundAmount, eligiblePlayers);
        }
    } else {
        // Side pots needed - use the full side pot creation logic
        sidePotManager.createSidePotsFromBets(playerBets);
        
        // Add folded player money to the main pot
        if (foldedPlayerMoney > 0) {
            sidePotManager.addToMainPot(foldedPlayerMoney);
        }
    }
}

void Table::returnUnmatchedChips(const std::vector<std::pair<int, int>>& playerBets) {
    // Find the maximum bet that can be matched by at least 2 players
    int maxMatchableBet = 0;
    for (const auto& playerBet : playerBets) {
        int potentialMax = playerBet.second;
        int playersWhoCanMatch = 0;
        for (const auto& otherBet : playerBets) {
            if (otherBet.second >= potentialMax) {
                playersWhoCanMatch++;
            }
        }
        if (playersWhoCanMatch >= 2) {
            maxMatchableBet = std::max(maxMatchableBet, potentialMax);
        }
    }
    
    // Return unmatched portions to players
    for (const auto& playerBet : playerBets) {
        if (playerBet.second > maxMatchableBet) {
            int unmatchedAmount = playerBet.second - maxMatchableBet;
            Player* player = getPlayer(playerBet.first);
            if (player) {
                player->addChips(unmatchedAmount);
                std::cout << player->getName() << " gets $" << unmatchedAmount 
                          << " returned (unmatched portion)" << std::endl;
            }
        }
    }
}

const SidePotManager& Table::getSidePotManager() const {
    return sidePotManager;
}

SidePotManager& Table::getSidePotManager() {
    return sidePotManager;
}

void Table::showPotBreakdown() const {
    sidePotManager.showPotBreakdown();
}

void Table::showTable() const {
    std::cout << "\n=== TABLE STATUS ===" << std::endl;
    std::cout << "Pot: $" << getPot() << " | Current Bet: $" << currentBet << std::endl;
    showCommunityCards();
    std::cout << "\nPlayers:" << std::endl;
    for (size_t i = 0; i < players.size(); i++) {
        std::cout << (i == static_cast<size_t>(dealerPosition) ? "[D] " : "    ");
        players[i]->showStatus(true);
    }
    std::cout << std::endl;
}

int Table::getDealerPosition() const {
    return dealerPosition;
}

void Table::advanceDealer() {
    dealerPosition = (dealerPosition + 1) % players.size();
}

Deck& Table::getDeck() {
    return deck;
} 