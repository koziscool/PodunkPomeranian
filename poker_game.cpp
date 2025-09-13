#include "poker_game.h"
#include <iostream>
#include <set>
#include <map>
#include <algorithm>

PokerGame::PokerGame(Table* gameTable, const VariantInfo& variant)
    : table(gameTable), variantInfo(variant), currentPlayerIndex(0), handComplete(false), 
      currentHandHasChoppedPot(false), handHistory(PokerVariant::TEXAS_HOLDEM, 1), 
      currentRound(UNIFIED_PRE_FLOP), betCount(0), currentActionPotIndex(0) {
    // TODO: HandHistory needs to be updated to use VariantInfo instead of PokerVariant
}

void PokerGame::showGameState() const {
    if (variantInfo.gameStruct == GAMESTRUCTURE_STUD) {
        table->showTableForStud();
    } else {
        table->showTable();
    }
}

std::vector<int> PokerGame::findWinners(const std::vector<int>& eligiblePlayers) {
    // Delegate to the new findBestHand method
    return findBestHand(eligiblePlayers);
}

// Common pot mechanics (same across all poker variants)
void PokerGame::collectBetsToInFor() {
    std::cout << "DEBUG: Starting collectBetsToInFor() - current action pot index: " << currentActionPotIndex << std::endl;
    
    // Show all player inFor amounts before processing
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            std::cout << "DEBUG: " << player->getName() << " has inFor: $" << player->getInFor() 
                     << " (folded: " << (player->hasFolded() ? "yes" : "no") 
                     << ", allIn: " << (player->isAllIn() ? "yes" : "no") << ")" << std::endl;
        }
    }
    
    // Check if anyone went all-in this round
    bool anyoneAllIn = false;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && player->isAllIn()) {
            anyoneAllIn = true;
            break;
        }
    }
    
    if (!anyoneAllIn) {
        // Simple case: no all-ins, just collect all money to current action pot
        int totalAmount = 0;
        std::set<int> eligiblePlayers;
        
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player && player->getInFor() > 0) {
                totalAmount += player->getInFor();
                if (!player->hasFolded()) {
                    eligiblePlayers.insert(i);
                }
            }
        }
        
        if (totalAmount > 0) {
            std::cout << "DEBUG: No all-ins, adding $" << totalAmount << " to current action pot " << currentActionPotIndex << std::endl;
            
            if (currentActionPotIndex == 0) {
                table->getSidePotManager().addToMainPot(totalAmount);
                table->getSidePotManager().addEligiblePlayersToMainPot(eligiblePlayers);
            } else {
                table->getSidePotManager().addSidePot(totalAmount, currentActionPotIndex, eligiblePlayers);
            }
        }
    } else {
        // Complex case: handle all-in side pots (we'll implement this properly later)
        std::cout << "DEBUG: All-in detected, complex side pot logic needed" << std::endl;
        // For now, just collect everything to main pot - we'll fix this later
        int totalAmount = 0;
        std::set<int> eligiblePlayers;
        
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player && player->getInFor() > 0) {
                totalAmount += player->getInFor();
                if (!player->hasFolded()) {
                    eligiblePlayers.insert(i);
                }
            }
        }
        
        if (totalAmount > 0) {
            table->getSidePotManager().addToMainPot(totalAmount);
            table->getSidePotManager().addEligiblePlayersToMainPot(eligiblePlayers);
        }
    }
    
    // Reset player bets and inFor
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            player->resetBet();
            player->resetInFor();
        }
    }
    table->setCurrentBet(0);
}

void PokerGame::handleAllInSidePots(int allInPlayerIndex) {
    Player* allInPlayer = table->getPlayer(allInPlayerIndex);
    if (!allInPlayer) return;
    
    int allInAmount = allInPlayer->getInFor();
    std::cout << "DEBUG: All-in amount = $" << allInAmount << std::endl;
    
    // Collect matching amounts from ALL active players for current action pot
    int potTotal = 0;
    std::set<int> eligiblePlayers;
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            // Every active player must match the all-in amount (up to what they have)
            int matchAmount = std::min(player->getInFor(), allInAmount);
            if (matchAmount > 0) {
                potTotal += matchAmount;
                std::cout << "DEBUG: Player " << player->getName() << " matches $" << matchAmount 
                         << " (had $" << player->getInFor() << ")" << std::endl;
                
                // Set player's inFor to remaining amount after match
                player->setInFor(player->getInFor() - matchAmount);
            } else {
                std::cout << "DEBUG: Player " << player->getName() << " has no inFor to match" << std::endl;
            }
            
            // All active players are eligible for this pot
            eligiblePlayers.insert(i);
        }
    }
    
    // Add matched amount to current action pot
    if (potTotal > 0) {
        std::cout << "DEBUG: Adding $" << potTotal << " to current action pot " << currentActionPotIndex << std::endl;
        if (currentActionPotIndex == 0) {
            // Adding to main pot
            table->getSidePotManager().addToMainPot(potTotal);
            table->getSidePotManager().addEligiblePlayersToMainPot(eligiblePlayers);
        } else {
            // Adding to existing side pot
            table->getSidePotManager().addToExistingSidePot(potTotal, eligiblePlayers);
        }
    }
    
    // Check if any players still have money left - if so, create new side pot
    bool hasRemainingMoney = false;
    std::set<int> remainingEligible;
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && player->getInFor() > 0 && !player->hasFolded()) {
            hasRemainingMoney = true;
            remainingEligible.insert(i);
        }
    }
    
    if (hasRemainingMoney && remainingEligible.size() > 1) {
        // Create new side pot and make it the current action pot
        currentActionPotIndex++;
        // The side pot will be created when we next call collectBetsToInFor()
    }
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
    table->setCurrentBet(amount);
    
    // Action display handled by completeBettingRound
    return true;
}

bool PokerGame::playerFold(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    PlayerAction action = player->fold();
    return action == PlayerAction::FOLD;
}

bool PokerGame::playerAllIn(int playerIndex) {
    Player* player = table->getPlayer(playerIndex);
    if (!player) return false;
    
    int allInAmount = player->getCurrentBet() + player->getChips();
    PlayerAction action = player->goAllIn();
    table->setCurrentBet(allInAmount);
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
    
    // Use Hi-Lo specific pot transfer for Hi-Lo games
    if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
        transferHiLoPotsToWinners(potAmount);
    } else {
        transferPotToWinners(potAmount, winners);
    }
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

void PokerGame::transferHiLoPotsToWinners(int potAmount) {
    if (hiWinners.empty() && loWinners.empty()) {
        std::cout << "No winners found for Hi-Lo pot!" << std::endl;
        return;
    }
    
    // Track if this is a chopped pot
    if (hiWinners.size() > 1 || loWinners.size() > 1 || (!loWinners.empty() && !hiWinners.empty())) {
        currentHandHasChoppedPot = true;
    }
    
    int totalAwarded = 0;
    
    if (loWinners.empty()) {
        // No qualifying low - all money goes to high winners
        std::cout << "No qualifying low hand - full pot goes to high" << std::endl;
        int amountPerHiWinner = potAmount / static_cast<int>(hiWinners.size());
        int remainder = potAmount % static_cast<int>(hiWinners.size());
        
        for (size_t i = 0; i < hiWinners.size(); i++) {
            Player* winner = table->getPlayer(hiWinners[i]);
            if (winner) {
                int award = amountPerHiWinner + (i < static_cast<size_t>(remainder) ? 1 : 0);
                winner->addChips(award);
                totalAwarded += award;
                std::cout << winner->getName() << " wins $" << award << " (high)" << std::endl;
            }
        }
    } else {
        // Split pot between high and low
        int hiHalf = potAmount / 2;
        int loHalf = potAmount - hiHalf;  // Handle odd amounts
        
        // Award high half
        if (!hiWinners.empty()) {
            int amountPerHiWinner = hiHalf / static_cast<int>(hiWinners.size());
            int hiRemainder = hiHalf % static_cast<int>(hiWinners.size());
            
            for (size_t i = 0; i < hiWinners.size(); i++) {
                Player* winner = table->getPlayer(hiWinners[i]);
                if (winner) {
                    int award = amountPerHiWinner + (i < static_cast<size_t>(hiRemainder) ? 1 : 0);
                    winner->addChips(award);
                    totalAwarded += award;
                    std::cout << winner->getName() << " wins $" << award << " (high)" << std::endl;
                }
            }
        }
        
        // Award low half
        int amountPerLoWinner = loHalf / static_cast<int>(loWinners.size());
        int loRemainder = loHalf % static_cast<int>(loWinners.size());
        
        for (size_t i = 0; i < loWinners.size(); i++) {
            Player* winner = table->getPlayer(loWinners[i]);
            if (winner) {
                int award = amountPerLoWinner + (i < static_cast<size_t>(loRemainder) ? 1 : 0);
                winner->addChips(award);
                totalAwarded += award;
                std::cout << winner->getName() << " wins $" << award << " (low)" << std::endl;
            }
        }
    }
    
    // Verify we awarded exactly the pot amount
    if (totalAwarded != potAmount) {
        std::cout << "ERROR: Pot awarding mismatch! Pot: $" << potAmount << ", Awarded: $" << totalAwarded << std::endl;
    }
}

// Utility functions for showdown
std::vector<int> PokerGame::findBestHand(const std::vector<int>& eligiblePlayers) {
    if (eligiblePlayers.empty()) {
        return {};
    }
    
    // Check if this is a hi-lo split pot variant
    if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
        return findHiLoWinners(eligiblePlayers);
    }
    
    // Standard high-only pot logic
    HandResult bestHand;
    bestHand.rank = HandRank::HIGH_CARD;
    std::vector<int> winners;
    
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult hand;
            
            // Use appropriate hand evaluation based on variant
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                // Omaha: must use exactly 2 hole + 3 community
                hand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
            } else {
                // Hold'em/Stud: use any 5 cards
                hand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
            }
            
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
    // Check if this is a hi-lo split pot variant
    if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
        displayHiLoWinningHands(winners, eligiblePlayers);
        return;
    }
    
    // Standard high-only display
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult hand;
            
            // Use appropriate hand evaluation based on variant (same as findBestHand)
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                // Omaha: must use exactly 2 hole + 3 community
                hand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
                
            } else {
                // Hold'em/Stud: use any 5 cards
                hand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
            }
            
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
    // TODO: Update HandHistory to use VariantInfo instead of PokerVariant
    handHistory = HandHistory(PokerVariant::TEXAS_HOLDEM, handNumber);
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
            // If player's inFor amount doesn't match current bet, betting isn't complete
            if (player->getInFor() < currentBet) {
                return false;
            }
            // If player hasn't acted this round, betting isn't complete
            // BUT: if player owes money, they MUST act regardless of hasActedThisRound
            if (!hasActedThisRound[i] && player->getInFor() >= currentBet) {
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
    betCount = 0;
}

void PokerGame::completeBettingRound(HandHistoryRound historyRound) {
    // Reset betting round state at the start of each betting round
    resetBettingRound();
    
    int actionCount = 0;
    const int MAX_ACTIONS = 150; // Allow for multiple raises with many players
    
    // Special handling for Stud third street - show bring-in as first action
    if (variantInfo.gameStruct == GAMESTRUCTURE_STUD && historyRound == HandHistoryRound::PRE_FLOP) {
        // Find the bring-in player and show the action
        int bringInPlayerIndex = -1;
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player && !player->hasFolded() && player->getInFor() > 0) {
                showGameState();
                std::cout << player->getName() << " brings in for $" << player->getInFor() << std::endl;
                bringInPlayerIndex = i;
                break;
            }
        }
        // Mark bring-in player as having acted AFTER the reset
        if (bringInPlayerIndex != -1) {
            hasActedThisRound[bringInPlayerIndex] = true;
        }
    }
    
    // Quick exit if only one player remains
    if (countActivePlayers() <= 1) {
        return;
    }
    
    while (!isBettingComplete() && actionCount < MAX_ACTIONS && countActivePlayers() > 1) {
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
        
        PlayerAction decision = player->makeDecision(handHistory, callAmount, canCheck, &variantInfo, betCount);
        
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
                actionDesc = "calls $" + std::to_string(player->getInFor());
                break;
            case PlayerAction::RAISE: {
                int raiseAmount = player->calculateRaiseAmount(handHistory, currentBet, variantInfo, currentRound);
                playerRaise(playerIndex, raiseAmount);
                if (currentBet == 0) {
                    actionDesc = "bets $" + std::to_string(raiseAmount);
                } else {
                    actionDesc = "raises to $" + std::to_string(raiseAmount);
                }
                // Increment bet count for limit games
                if (variantInfo.bettingStruct == BETTINGSTRUCTURE_LIMIT) {
                    betCount++;
                }
                
                // When someone raises, players who now owe money need to act again
                int newCurrentBet = table->getCurrentBet();
                for (int i = 0; i < table->getPlayerCount(); i++) {
                    Player* otherPlayer = table->getPlayer(i);
                    if (otherPlayer && !otherPlayer->hasFolded() && !otherPlayer->isAllIn() && 
                        i != playerIndex && otherPlayer->getInFor() < newCurrentBet) {
                        hasActedThisRound[i] = false; // They need to act again
                    }
                }
                break;
            }
            case PlayerAction::ALL_IN:
                playerAllIn(playerIndex);
                actionDesc = "goes all-in for $" + std::to_string(player->getInFor());
                std::cout << "DEBUG: " << player->getName() << " went all-in for $" << player->getInFor() << std::endl;
                break;
        }
        
        // Display the action
        std::cout << player->getName() << " " << actionDesc << std::endl;
        
        // Record the action in hand history
        recordPlayerAction(historyRound, player->getPlayerId(), 
                          static_cast<ActionType>(decision), 
                          (decision == PlayerAction::RAISE) ? player->calculateRaiseAmount(handHistory, currentBet, variantInfo, currentRound) : callAmount,
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

// Unified game flow implementation
void PokerGame::startNewHand() {
    currentRound = UNIFIED_PRE_FLOP;
    handComplete = false;
    currentHandHasChoppedPot = false;
    hasActedThisRound.assign(table->getPlayerCount(), false);
    currentActionPotIndex = 0; // All money goes to main pot initially
    
    // Initialize hand history
    initializeHandHistory(1);
    
    // Deal initial cards based on variant
    dealInitialCards();
    
    // Post forced bets based on game structure
    if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD) {
        postBlinds();
    } else if (variantInfo.gameStruct == GAMESTRUCTURE_STUD) {
        postAntesAndBringIn();
    }
    
    // Set initial player to act
    if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD) {
        // UTG (3 seats after dealer for board games)
        int dealerPos = table->getDealerPosition();
        currentPlayerIndex = (dealerPos + 3) % table->getPlayerCount();
    } else {
        // Stud: player with bring-in acts first (set in postAntesAndBringIn)
    }
}

void PokerGame::dealInitialCards() {
    if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD) {
        // Deal hole cards based on variant
        int numHoleCards = (variantInfo.numHoleCards == NUMHOLECARDS_TWO) ? 2 : 4;
        for (int round = 0; round < numHoleCards; round++) {
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player) {
                    Card card = table->getDeck().dealCard();
                    player->addCard(card); // Face down by default
                }
            }
        }
    } else if (variantInfo.gameStruct == GAMESTRUCTURE_STUD) {
        // Deal 2 down, 1 up for seven card stud
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player) {
                // Two down cards
                player->addCard(table->getDeck().dealCard(), false);
                player->addCard(table->getDeck().dealCard(), false);
                // One up card
                player->addCard(table->getDeck().dealCard(), true);
            }
        }
        // Mark initial 3rd street cards as not "new" so they don't get asterisks
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player) {
                player->markStartOfStreet(); // Sets cardsAtStartOfStreet to current hand size (3)
            }
        }
    }
}

void PokerGame::runBettingRounds() {
    switch (variantInfo.gameStruct) {
        case GAMESTRUCTURE_BOARD:
            gameFlowForBOARD();
            break;
        case GAMESTRUCTURE_STUD:
            gameFlowForSTUD();
            break;
    }
    
    // Conduct showdown if we reached it
    if (atShowdown()) {
        conductShowdown();
    }
}

void PokerGame::gameFlowForBOARD() {
    while (!isHandComplete()) {
        if (currentRound == UNIFIED_PRE_FLOP) {
            std::cout << "\n=== PRE-FLOP ===" << std::endl;
            showGameState();
            completeBettingRound(HandHistoryRound::PRE_FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_FLOP) {
            std::cout << "\n=== FLOP ===" << std::endl;
            table->dealFlop();
            showGameState();
            completeBettingRound(HandHistoryRound::FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_TURN) {
            std::cout << "\n=== TURN ===" << std::endl;
            table->dealTurn();
            showGameState();
            completeBettingRound(HandHistoryRound::TURN);
            nextRound();
        } else if (currentRound == UNIFIED_RIVER) {
            std::cout << "\n=== RIVER ===" << std::endl;
            table->dealRiver();
            showGameState();
            completeBettingRound(HandHistoryRound::RIVER);
            currentRound = UNIFIED_SHOWDOWN;
        }
    }
}

void PokerGame::gameFlowForSTUD() {
    while (!isHandComplete()) {
        if (currentRound == UNIFIED_PRE_FLOP) { // Third street
            std::cout << "\n=== THIRD STREET ===" << std::endl;
            // Don't show game state before betting - bring-in will be shown as first action
            completeBettingRound(HandHistoryRound::PRE_FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_FLOP) { // Fourth street
            std::cout << "\n=== FOURTH STREET ===" << std::endl;
            // Mark start of new street for all players
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->markStartOfStreet();
                }
            }
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            // Set betting order based on highest up cards
            currentPlayerIndex = findStudFirstToAct();
            completeBettingRound(HandHistoryRound::FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_TURN) { // Fifth street
            std::cout << "\n=== FIFTH STREET ===" << std::endl;
            // Mark start of new street for all players
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->markStartOfStreet();
                }
            }
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            // Set betting order based on highest up cards
            currentPlayerIndex = findStudFirstToAct();
            completeBettingRound(HandHistoryRound::TURN);
            nextRound();
        } else if (currentRound == UNIFIED_RIVER) { // Sixth street
            std::cout << "\n=== SIXTH STREET ===" << std::endl;
            // Mark start of new street for all players
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->markStartOfStreet();
                }
            }
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            // Set betting order based on highest up cards
            currentPlayerIndex = findStudFirstToAct();
            completeBettingRound(HandHistoryRound::RIVER);
            nextRound();
        } else if (currentRound == UNIFIED_FINAL) { // Seventh street
            std::cout << "\n=== SEVENTH STREET ===" << std::endl;
            // Mark start of new street for all players
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->markStartOfStreet();
                }
            }
            // Deal final down card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), false);
                }
            }
            showGameState();
            // Set betting order based on highest up cards (same as 6th street)
            currentPlayerIndex = findStudFirstToAct();
            completeBettingRound(HandHistoryRound::SHOWDOWN);
            currentRound = UNIFIED_SHOWDOWN;
        }
    }
}

void PokerGame::postBlinds() {
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    
    // Small blind (next after dealer)
    int smallBlindPos = (dealerPos + 1) % playerCount;
    Player* smallBlindPlayer = table->getPlayer(smallBlindPos);
    
    // Big blind (next after small blind)
    int bigBlindPos = (dealerPos + 2) % playerCount;
    Player* bigBlindPlayer = table->getPlayer(bigBlindPos);
    
    if (smallBlindPlayer && bigBlindPlayer) {
        int smallBlind = variantInfo.betSizes[0];
        int bigBlind = variantInfo.betSizes[1];
        
        smallBlindPlayer->addToInFor(smallBlind);
        table->setCurrentBet(smallBlind);
        
        recordPlayerAction(HandHistoryRound::PRE_HAND, smallBlindPlayer->getPlayerId(),
                          ActionType::POST_BLIND, smallBlind,
                          "posts small blind $" + std::to_string(smallBlind));
        
        bigBlindPlayer->addToInFor(bigBlind);
        table->setCurrentBet(bigBlind);
        
        recordPlayerAction(HandHistoryRound::PRE_HAND, bigBlindPlayer->getPlayerId(),
                          ActionType::POST_BLIND, bigBlind,
                          "posts big blind $" + std::to_string(bigBlind));
        
        std::cout << smallBlindPlayer->getName() << " posts SB $" << smallBlind 
                  << ", " << bigBlindPlayer->getName() << " posts BB $" << bigBlind << std::endl;
    }
}

void PokerGame::postAntesAndBringIn() {
    int ante = variantInfo.betSizes[0];
    int bringIn = variantInfo.betSizes[1];
    
    std::cout << "All players ante $" << ante << std::endl;
    
    // Collect antes from all players - goes directly to pot, not inFor
    int totalAntes = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            player->deductChips(ante);  // Take chips but don't add to inFor
            totalAntes += ante;
            recordPlayerAction(HandHistoryRound::PRE_HAND, player->getPlayerId(),
                              ActionType::POST_BLIND, ante,
                              "posts ante $" + std::to_string(ante));
        }
    }
    
    // Add antes directly to the main pot
    table->getSidePotManager().addToMainPot(totalAntes);
    
    // Find player with lowest up card for bring-in
    int bringInPlayer = -1;
    Card lowestCard(Suit::SPADES, Rank::ACE); // Start with highest possible
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            Card upCard = player->getLowestUpCard();
            if (static_cast<int>(upCard.getRank()) < static_cast<int>(lowestCard.getRank()) ||
                (upCard.getRank() == lowestCard.getRank() && static_cast<int>(upCard.getSuit()) < static_cast<int>(lowestCard.getSuit()))) {
                lowestCard = upCard;
                bringInPlayer = i;
            }
        }
    }
    
    if (bringInPlayer != -1) {
        Player* bringInPlayerPtr = table->getPlayer(bringInPlayer);
        bringInPlayerPtr->addToInFor(bringIn);
        table->setCurrentBet(bringIn);
        
        recordPlayerAction(HandHistoryRound::PRE_FLOP, bringInPlayerPtr->getPlayerId(),
                          ActionType::POST_BLIND, bringIn,
                          "brings in for $" + std::to_string(bringIn));
        
        // Don't show bring-in immediately - will be shown as first betting action
        
        // First to act is next player after bring-in
        currentPlayerIndex = (bringInPlayer + 1) % table->getPlayerCount();
    }
}

int PokerGame::findStudFirstToAct() const {
    // For 4th street and beyond: player with highest up cards acts first
    int bestPlayerIndex = -1;
    std::vector<Card> bestUpCards;
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            std::vector<Card> upCards = player->getUpCards();
            if (!upCards.empty()) {
                // Simple comparison for Stud: pairs beat high card, higher pairs beat lower pairs
                if (bestPlayerIndex == -1 || determineBettorForStud(upCards, bestUpCards)) {
                    bestUpCards = upCards;
                    bestPlayerIndex = i;
                }
            }
        }
    }
    
    return bestPlayerIndex;
}

bool PokerGame::determineBettorForStud(const std::vector<Card>& hand1, const std::vector<Card>& hand2) const {
    // Analyze both hands for Stud betting order: 4-of-a-kind > trips > 2-pair > 1-pair > high card
    auto analyzeHand = [](const std::vector<Card>& hand) {
        std::map<Rank, int> counts;
        for (const Card& card : hand) {
            counts[card.getRank()]++;
        }
        
        // Find pairs, trips, quads and categorize hand
        std::vector<std::pair<int, int>> pairs; // (count, rank)
        std::vector<int> kickers;
        
        for (const auto& entry : counts) {
            int count = entry.second;
            int rank = static_cast<int>(entry.first);
            
            if (count >= 2) {
                pairs.push_back({count, rank});
            } else {
                kickers.push_back(rank);
            }
        }
        
        // Sort pairs by count first, then by rank (descending)
        std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
            if (a.first != b.first) return a.first > b.first; // Higher count first
            return a.second > b.second; // Higher rank first
        });
        
        // Sort kickers descending
        std::sort(kickers.rbegin(), kickers.rend());
        
        return std::make_pair(pairs, kickers);
    };
    
    auto result1 = analyzeHand(hand1);
    auto result2 = analyzeHand(hand2);
    auto pairs1 = result1.first;
    auto kickers1 = result1.second;
    auto pairs2 = result2.first;
    auto kickers2 = result2.second;
    
    // Compare hand types first
    // Determine hand type: 0=high card, 1=one pair, 2=two pair, 3=trips, 4=quads
    auto getHandType = [](const std::vector<std::pair<int, int>>& pairs) {
        if (pairs.empty()) return 0; // high card
        if (pairs[0].first == 4) return 4; // quads
        if (pairs[0].first == 3) return 3; // trips
        if (pairs.size() >= 2) return 2; // two pair
        if (pairs[0].first == 2) return 1; // one pair
        return 0;
    };
    
    int type1 = getHandType(pairs1);
    int type2 = getHandType(pairs2);
    
    if (type1 != type2) {
        return type1 > type2;
    }
    
    // Same hand type - compare within type
    if (type1 >= 1) { // Has pairs/trips/quads
        // Compare primary pairs/trips/quads first
        for (size_t i = 0; i < std::min(pairs1.size(), pairs2.size()); i++) {
            if (pairs1[i].second != pairs2[i].second) {
                return pairs1[i].second > pairs2[i].second;
            }
        }
        // If pairs are equal, compare kickers
    }
    
    // Compare kickers (or high cards if no pairs)
    size_t minKickers = std::min(kickers1.size(), kickers2.size());
    for (size_t i = 0; i < minKickers; i++) {
        if (kickers1[i] != kickers2[i]) {
            return kickers1[i] > kickers2[i];
        }
    }
    
    // If all compared cards equal, more cards is better
    return hand1.size() > hand2.size();
}

void PokerGame::nextRound() {
    if (currentRound == UNIFIED_PRE_FLOP) {
        currentRound = UNIFIED_FLOP;
    } else if (currentRound == UNIFIED_FLOP) {
        currentRound = UNIFIED_TURN;
    } else if (currentRound == UNIFIED_TURN) {
        currentRound = UNIFIED_RIVER;
    } else if (currentRound == UNIFIED_RIVER) {
        if (variantInfo.gameStruct == GAMESTRUCTURE_STUD) {
            currentRound = UNIFIED_FINAL; // Stud has seventh street
        } else {
            currentRound = UNIFIED_SHOWDOWN; // Board games done after river
        }
    } else if (currentRound == UNIFIED_FINAL) {
        currentRound = UNIFIED_SHOWDOWN;
    }
    
    // Reset current bet for next round
    table->setCurrentBet(0);
    
    // Set first to act (different for BOARD vs STUD)
    if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD) {
        // Post-flop: first active player after dealer button
        int dealerPos = table->getDealerPosition();
        currentPlayerIndex = dealerPos; // Start at dealer
        advanceToNextPlayer(); // Find first active player after dealer
    } else {
        // For stud, highest up cards act first (simplified: just use first active player)
        currentPlayerIndex = 0;
        advanceToNextPlayer(); // Find first active player
    }
}

void PokerGame::conductShowdown() {
    std::cout << "\n=== SHOWDOWN ===" << std::endl;
    
    // Show all players' cards (for board games only - Stud cards are already visible)
    if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD) {
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player && !player->hasFolded()) {
                std::cout << player->getName() << " hole cards: ";
                for (const auto& card : player->getHand()) {
                    std::cout << card.toString() << " ";
                }
                std::cout << std::endl;
            }
        }
    }
    
    awardPotsStaged();
}

void PokerGame::awardPotsWithoutShowdown() {
    // Find the single remaining active player
    int remainingPlayer = -1;
    int activeCount = 0;
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            remainingPlayer = i;
            activeCount++;
        }
    }
    
    if (activeCount != 1) {
        std::cout << "Error: awardPotsWithoutShowdown called but " << activeCount << " players remain active!" << std::endl;
        return;
    }
    
    Player* winner = table->getPlayer(remainingPlayer);
    if (!winner) {
        std::cout << "Error: Could not find remaining player!" << std::endl;
        return;
    }
    
    const auto& pots = table->getSidePotManager().getPots();
    int totalWinnings = 0;
    
    std::cout << "\n=== ALL OTHER PLAYERS FOLDED ===" << std::endl;
    std::cout << winner->getName() << " wins by default!" << std::endl;
    
    // Award all pots to the remaining player
    for (size_t i = 0; i < pots.size(); i++) {
        const SidePot& pot = pots[i];
        
        // Check if this player is eligible for this pot
        if (pot.eligiblePlayers.find(remainingPlayer) != pot.eligiblePlayers.end()) {
            std::cout << "Awarding ";
            if (i == 0) {
                std::cout << "main pot";
            } else {
                std::cout << "side pot " << i;
            }
            std::cout << " ($" << pot.amount << ") to " << winner->getName() << std::endl;
            
            winner->addChips(pot.amount);
            totalWinnings += pot.amount;
        }
    }
    
    std::cout << winner->getName() << " total winnings: $" << totalWinnings << std::endl;
    std::cout << winner->getName() << " now has $" << winner->getChips() << std::endl;
}

bool PokerGame::atShowdown() const {
    return currentRound == UNIFIED_SHOWDOWN;
}

// Omaha hand evaluation (exactly 2 hole + 3 community)
HandResult PokerGame::evaluateOmahaHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const {
    HandResult bestHand;
    bestHand.rank = HandRank::HIGH_CARD;
    
    if (holeCards.size() < 2 || communityCards.size() < 3) {
        return bestHand; // Not enough cards
    }
    
    // Try all combinations of exactly 2 hole cards + 3 community cards
    for (size_t h1 = 0; h1 < holeCards.size(); h1++) {
        for (size_t h2 = h1 + 1; h2 < holeCards.size(); h2++) {
            for (size_t c1 = 0; c1 < communityCards.size(); c1++) {
                for (size_t c2 = c1 + 1; c2 < communityCards.size(); c2++) {
                    for (size_t c3 = c2 + 1; c3 < communityCards.size(); c3++) {
                        // Create 5-card hand: exactly 2 hole + 3 community
                        std::vector<Card> fiveCards = {
                            holeCards[h1], holeCards[h2],
                            communityCards[c1], communityCards[c2], communityCards[c3]
                        };
                        
                        // Evaluate this 5-card combination
                        HandResult result = HandEvaluator::evaluateHand(fiveCards, {});
                        
                        if (result > bestHand) {
                            bestHand = result;
                            // Ensure the bestHand field contains the actual 5 cards used
                            bestHand.bestHand = fiveCards;
                        }
                    }
                }
            }
        }
    }
    
    return bestHand;
}

LowHandResult PokerGame::evaluateOmahaLowHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const {
    if (holeCards.size() != 4 || communityCards.size() != 5) {
        return LO_HAND_UNQUALIFIED;
    }
    
    LowHandResult bestLow = LO_HAND_UNQUALIFIED;
    
    // Generate all combinations of exactly 2 hole cards and 3 community cards
    for (int h1 = 0; h1 < 4; h1++) {
        for (int h2 = h1 + 1; h2 < 4; h2++) {
            for (int c1 = 0; c1 < 5; c1++) {
                for (int c2 = c1 + 1; c2 < 5; c2++) {
                    for (int c3 = c2 + 1; c3 < 5; c3++) {
                        std::vector<Card> fiveCards = {
                            holeCards[h1], holeCards[h2],
                            communityCards[c1], communityCards[c2], communityCards[c3]
                        };
                        
                        LowHandResult result = HandEvaluator::evaluate5CardsForLowA5(fiveCards);
                        
                        // Since we're using LO_HAND_UNQUALIFIED constant, we can compare directly
                        if (result < bestLow) {
                            bestLow = result;
                            // Ensure the bestLowHand field contains the actual 5 cards used
                            if (result.qualified) {
                                bestLow.bestLowHand = fiveCards;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return bestLow;
}

std::vector<int> PokerGame::findHiLoWinners(const std::vector<int>& eligiblePlayers) {
    if (eligiblePlayers.empty()) {
        return {};
    }
    
    // Find high hand winners
    HandResult bestHighHand;
    bestHighHand.rank = HandRank::HIGH_CARD;
    std::vector<int> highWinners;
    
    // Find low hand winners
    LowHandResult bestLowHand = LO_HAND_UNQUALIFIED;
    std::vector<int> lowWinners;
    
    // Evaluate all eligible players for both high and low
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            
            // Evaluate high hand
            HandResult highHand;
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                // Omaha: must use exactly 2 hole + 3 community
                highHand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
            } else {
                // Hold'em/Stud: use any 5 cards
                highHand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
            }
            
            if (highHand > bestHighHand) {
                bestHighHand = highHand;
                highWinners.clear();
                highWinners.push_back(playerIndex);
            } else if (highHand == bestHighHand) {
                highWinners.push_back(playerIndex);
            }
            
            // Evaluate low hand
            LowHandResult lowHand;
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                // Omaha: must use exactly 2 hole + 3 community for low
                lowHand = evaluateOmahaLowHand(player->getHand(), table->getCommunityCards());
            } else {
                // Hold'em/Stud: use any 5 cards for low
                lowHand = HandEvaluator::evaluateLowHand(player->getHand(), table->getCommunityCards());
            }
            
            // Apply 8-or-better qualification for HILO_A5_MUSTQUALIFY games
            if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
                // Check if hand qualifies: no pairs AND highest card is 8 or lower
                bool hasNoPairs = (lowHand.values.size() >= 6 && lowHand.values[0] == 0); // handType == 0
                bool highCardIsEightOrLower = (lowHand.values.size() >= 6 && lowHand.values[1] <= 8); // highest card <= 8
                
                if (!hasNoPairs || !highCardIsEightOrLower) {
                    lowHand = LO_HAND_UNQUALIFIED; // Hand doesn't qualify for low
                }
            }
            
            // With LO_HAND_UNQUALIFIED, we can simply compare all low hands
            if (lowHand < bestLowHand) {
                bestLowHand = lowHand;
                lowWinners.clear();
                lowWinners.push_back(playerIndex);
            } else if (lowHand == bestLowHand) {
                lowWinners.push_back(playerIndex);
            }
        }
    }
    
    // Store both high and low winners for proper Hi-Lo pot splitting
    // We'll handle the actual split in transferHiLoPotsToWinners
    hiWinners = highWinners;
    loWinners = (bestLowHand.qualified) ? lowWinners : std::vector<int>();
    
    // For compatibility, return combined list but actual pot splitting will use hiWinners/loWinners
    std::vector<int> allWinners = highWinners;
    if (bestLowHand.qualified) {
        for (int lowWinner : lowWinners) {
            if (std::find(allWinners.begin(), allWinners.end(), lowWinner) == allWinners.end()) {
                allWinners.push_back(lowWinner);
            }
        }
    }
    
    return allWinners;
}

void PokerGame::displayHiLoWinningHands(const std::vector<int>& /* winners */, const std::vector<int>& eligiblePlayers) const {
    // Re-evaluate to determine high and low winners separately
    HandResult bestHighHand;
    bestHighHand.rank = HandRank::HIGH_CARD;
    std::vector<int> highWinners;
    
    LowHandResult bestLowHand = LO_HAND_UNQUALIFIED;
    std::vector<int> lowWinners;
    
    // Find the actual high and low winners
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            
            // Evaluate high hand
            HandResult highHand;
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                highHand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
            } else {
                highHand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
            }
            
            if (highHand > bestHighHand) {
                bestHighHand = highHand;
                highWinners.clear();
                highWinners.push_back(playerIndex);
            } else if (highHand == bestHighHand) {
                highWinners.push_back(playerIndex);
            }
            
            // Evaluate low hand
            LowHandResult lowHand;
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                lowHand = evaluateOmahaLowHand(player->getHand(), table->getCommunityCards());
            } else {
                lowHand = HandEvaluator::evaluateLowHand(player->getHand(), table->getCommunityCards());
            }
            
            // Apply 8-or-better qualification for winner determination
            if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
                bool hasNoPairs = (lowHand.values.size() >= 6 && lowHand.values[0] == 0);
                bool highCardIsEightOrLower = (lowHand.values.size() >= 6 && lowHand.values[1] <= 8);
                
                if (!hasNoPairs || !highCardIsEightOrLower) {
                    lowHand = LO_HAND_UNQUALIFIED;
                }
            }
            
            // Simplified comparison using LO_HAND_UNQUALIFIED
            if (lowHand < bestLowHand) {
                bestLowHand = lowHand;
                lowWinners.clear();
                lowWinners.push_back(playerIndex);
            } else if (lowHand == bestLowHand) {
                lowWinners.push_back(playerIndex);
            }
        }
    }
    
    // Display all hands with appropriate winner indicators
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult highHand;
            LowHandResult lowHand;
            
            // Evaluate both hands for display
            if (variantInfo.handResolution == BESTHANDRESOLUTION_TWOPLUSTHREE) {
                highHand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
                lowHand = evaluateOmahaLowHand(player->getHand(), table->getCommunityCards());
            } else {
                highHand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
                lowHand = HandEvaluator::evaluateLowHand(player->getHand(), table->getCommunityCards());
            }
            
            // Apply 8-or-better qualification for display
            if (variantInfo.potResolution == POTRESOLUTION_HILO_A5_MUSTQUALIFY) {
                bool hasNoPairs = (lowHand.values.size() >= 6 && lowHand.values[0] == 0);
                bool highCardIsEightOrLower = (lowHand.values.size() >= 6 && lowHand.values[1] <= 8);
                
                if (!hasNoPairs || !highCardIsEightOrLower) {
                    lowHand = LO_HAND_UNQUALIFIED;
                }
            }
            
            bool isHighWinner = std::find(highWinners.begin(), highWinners.end(), playerIndex) != highWinners.end();
            bool isLowWinner = std::find(lowWinners.begin(), lowWinners.end(), playerIndex) != lowWinners.end();
            
            std::cout << player->getName() << ":" << std::endl;
            std::cout << "  High: " << highHand.description;
            if (isHighWinner) {
                std::cout << " (HIGH WINNER)";
            }
            std::cout << std::endl;
            
            // Display low hand with proper qualification messaging
            std::cout << "  Low: ";
            if (lowHand.qualified) {
                std::cout << lowHand.description;
                if (isLowWinner) {
                    std::cout << " (LOW WINNER)";
                }
            } else {
                std::cout << "No qualifying low";
            }
            std::cout << std::endl;
        }
    }
    
    // Summary of pot split
    if (bestLowHand.qualified) {
        std::cout << "\n=== POT SPLIT ===\n";
        std::cout << "High half goes to: ";
        for (size_t i = 0; i < highWinners.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << table->getPlayer(highWinners[i])->getName();
        }
        std::cout << "\nLow half goes to: ";
        for (size_t i = 0; i < lowWinners.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << table->getPlayer(lowWinners[i])->getName();
        }
        std::cout << std::endl;
    } else {
        std::cout << "\n=== NO QUALIFYING LOW ===\n";
        std::cout << "Entire pot goes to high winners: ";
        for (size_t i = 0; i < highWinners.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << table->getPlayer(highWinners[i])->getName();
        }
        std::cout << std::endl;
    }
}