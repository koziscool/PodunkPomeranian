#include "side_pot.h"
#include <iostream>
#include <algorithm>

void SidePotManager::clearPots() {
    pots.clear();
}

void SidePotManager::createSidePotsFromBets(const std::vector<std::pair<int, int>>& playerBets) {
    createSidePotsFromBets(playerBets, true); // Default behavior: clear existing pots
}

void SidePotManager::createSidePotsFromBets(const std::vector<std::pair<int, int>>& playerBets, bool clearExisting) {
    if (clearExisting) {
        clearPots();
    }
    
    if (playerBets.empty()) {
        return;
    }
    
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
    
    // Cap all bets at the maximum matchable amount
    std::vector<std::pair<int, int>> cappedBets;
    for (const auto& playerBet : playerBets) {
        int cappedAmount = std::min(playerBet.second, maxMatchableBet);
        if (cappedAmount > 0) {
            cappedBets.emplace_back(playerBet.first, cappedAmount);
        }
    }
    
    // Collect unique bet levels and sort them (ascending)
    std::set<int> betLevels;
    for (const auto& playerBet : cappedBets) {
        if (playerBet.second > 0) {
            betLevels.insert(playerBet.second);
        }
    }
    
    std::vector<int> sortedLevels(betLevels.begin(), betLevels.end());
    std::sort(sortedLevels.begin(), sortedLevels.end());
    
    // Create pots starting from the smallest bet amount
    // Main pot first, then side pots for each additional level
    int previousLevel = 0;
    for (int currentLevel : sortedLevels) {
        int potContribution = currentLevel - previousLevel;
        
        // Find all players who bet at least this level
        std::set<int> eligiblePlayers;
        for (const auto& playerBet : cappedBets) {
            if (playerBet.second >= currentLevel) {
                eligiblePlayers.insert(playerBet.first);
            }
        }
        
        // Only create pot if more than one player is eligible
        if (eligiblePlayers.size() > 1) {
            int potAmount = potContribution * static_cast<int>(eligiblePlayers.size());
            pots.emplace_back(potAmount, currentLevel);
            pots.back().eligiblePlayers = eligiblePlayers;
            

        }
        
        previousLevel = currentLevel;
    }
}

void SidePotManager::addToPot(int amount, const std::set<int>& eligiblePlayers) {
    if (pots.empty()) {
        pots.emplace_back(amount, 0);
        pots.back().eligiblePlayers = eligiblePlayers;
    } else {
        pots[0].amount += amount;
        // Add eligible players to existing pot
        for (int player : eligiblePlayers) {
            pots[0].eligiblePlayers.insert(player);
        }
    }
}

void SidePotManager::addToMainPot(int amount) {
    // If there's already a main pot (first pot), add to it
    if (!pots.empty()) {
        pots[0].amount += amount;
    } else {
        // Create a new main pot - eligibility will be set when active players create pots
        std::set<int> emptyEligibility;
        pots.emplace_back(amount, 0);
        pots.back().eligiblePlayers = emptyEligibility;
    }
}

void SidePotManager::addEligiblePlayersToMainPot(const std::set<int>& players) {
    if (!pots.empty()) {
        for (int playerIndex : players) {
            pots[0].eligiblePlayers.insert(playerIndex);
        }
    }
}

int SidePotManager::getTotalPotAmount() const {
    int total = 0;
    for (const auto& pot : pots) {
        total += pot.amount;
    }
    return total;
}

int SidePotManager::getMainPotAmount() const {
    return pots.empty() ? 0 : pots[0].amount;
}

int SidePotManager::getNumberOfPots() const {
    return static_cast<int>(pots.size());
}

const std::vector<SidePot>& SidePotManager::getPots() const {
    return pots;
}

void SidePotManager::showPotBreakdown() const {
    if (pots.empty()) {
        std::cout << "No pots created yet." << std::endl;
        return;
    }
    
    for (size_t i = 0; i < pots.size(); i++) {
        const auto& pot = pots[i];
        if (i == 0) {
            std::cout << "Main Pot: $" << pot.amount;
        } else {
            std::cout << "  |  Side Pot " << i << ": $" << pot.amount;
        }
    }
    std::cout << std::endl;
} 