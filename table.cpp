#include "table.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <set> // Added for std::set

Table::Table() : dealerPosition(0), currentBet(0) {
    deck.shuffle();
}

void Table::addPlayer(const std::string& name, int chips, int playerId, PlayerPersonality personality) {
    players.push_back(std::make_unique<Player>(name, chips, playerId, personality));
}

void Table::removePlayer(int index) {
    if (index >= 0 && index < static_cast<int>(players.size())) {
        // Adjust dealer position if necessary
        if (index <= dealerPosition && dealerPosition > 0) {
            dealerPosition--;
        }
        // Remove the player
        players.erase(players.begin() + index);
        // Ensure dealer position is still valid
        if (dealerPosition >= static_cast<int>(players.size()) && !players.empty()) {
            dealerPosition = 0;
        }
    }
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

void Table::clearCommunityCards() {
    communityCards.clear();
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
    // Matching-based pot allocation logic
    // Process all-ins once at end of betting round
    
    // Find all unique all-in amounts and sort from smallest to largest
    std::set<int> allInLevels;
    for (int i = 0; i < getPlayerCount(); i++) {
        Player* player = getPlayer(i);
        if (player && player->isAllIn() && player->getInFor() > 0) {
            allInLevels.insert(player->getInFor());
        }
    }
    
    // If no all-in players, just add everything to main pot
    if (allInLevels.empty()) {
        int totalAmount = 0;
        std::set<int> eligiblePlayers;
        
        // Collect all money from all players (including folded)
        for (int i = 0; i < getPlayerCount(); i++) {
            Player* player = getPlayer(i);
            if (player && player->getInFor() > 0) {
                totalAmount += player->getInFor();
                if (!player->hasFolded()) {
                    eligiblePlayers.insert(i);
                }
            }
        }
        
        // Add to existing pot or create new one
        if (!sidePotManager.getPots().empty()) {
            sidePotManager.addToMainPot(totalAmount);
            sidePotManager.addEligiblePlayersToMainPot(eligiblePlayers);
        } else if (totalAmount > 0) {
            sidePotManager.addToPot(totalAmount, eligiblePlayers);
        }
        
        return;
    }
    
    // Process each all-in level using matching logic
    int previousLevel = 0;
    bool isFirstPot = sidePotManager.getPots().empty();
    
    for (int allInLevel : allInLevels) {
        int matchAmount = allInLevel - previousLevel;
        if (matchAmount <= 0) continue;
        
        std::set<int> eligiblePlayers;
        int potTotal = 0;
        
        // Match this amount from all players who have at least this much
        // This includes both live players and folded players
        for (int i = 0; i < getPlayerCount(); i++) {
            Player* player = getPlayer(i);
            if (player && player->getInFor() >= allInLevel) {
                // This player contributed to this pot level
                potTotal += matchAmount;
                
                // Only live players are eligible to win
                if (!player->hasFolded()) {
                    eligiblePlayers.insert(i);
                }
            }
        }
        
        // Create pot if there's money and eligible players
        if (potTotal > 0 && !eligiblePlayers.empty()) {
            if (isFirstPot) {
                // First pot becomes main pot
                sidePotManager.addToPot(potTotal, eligiblePlayers);
                isFirstPot = false;
            } else {
                // For all-in situations, first all-in level should always be added to main pot
                // Additional levels become side pots
                if (previousLevel == 0) {
                    // This is the first (lowest) all-in level - add to main pot
                    sidePotManager.addToPot(potTotal, eligiblePlayers);
                } else {
                    // This is a higher all-in level - create side pot
                    sidePotManager.addSidePot(potTotal, allInLevel, eligiblePlayers);
                }
            }
        }
        
        previousLevel = allInLevel;
    }
    
    // Handle remaining money from players who contributed more than highest all-in
    int highestAllIn = allInLevels.empty() ? 0 : *allInLevels.rbegin();
    std::set<int> remainingEligible;
    int remainingTotal = 0;
    
    for (int i = 0; i < getPlayerCount(); i++) {
        Player* player = getPlayer(i);
        if (player && player->getInFor() > highestAllIn && !player->hasFolded()) {
            remainingEligible.insert(i);
            remainingTotal += (player->getInFor() - highestAllIn);
        }
    }
    
    if (remainingTotal > 0 && remainingEligible.size() > 1) {
        // Try to add to existing side pot first, if not possible create new one
        if (!sidePotManager.addToExistingSidePot(remainingTotal, remainingEligible)) {
            // Create new side pot for remaining active players
            sidePotManager.addSidePot(remainingTotal, highestAllIn + 1, remainingEligible);
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

void Table::showTableForStud() const {
    std::cout << "\n=== TABLE STATUS ===" << std::endl;
    std::cout << "Pot: $" << getPot() << " | Current Bet: $" << currentBet << std::endl;
    std::cout << "\nPlayers:" << std::endl;
    for (size_t i = 0; i < players.size(); i++) {
        std::cout << "    "; // No dealer button for Stud
        
        // Show player info without cards
        Player* player = players[i].get();
        std::cout << std::setw(15) << player->getName()
                  << " | Chips: " << std::setw(6) << player->getChips()
                  << " | Bet: " << std::setw(4) << player->getInFor()
                  << " | Cards: ";
        
        // Use Stud-specific hand display with new cards marked
        player->showStudHandWithNew();
        
        if (player->hasFolded()) {
            std::cout << " | FOLDED";
        } else if (player->isAllIn()) {
            std::cout << " | ALL-IN";
        }
        std::cout << std::endl;
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