#include "poker_game.h"
#include <iostream>
#include <set>
#include <algorithm>

PokerGame::PokerGame(Table* gameTable, const VariantInfo& variant)
    : table(gameTable), variantInfo(variant), currentPlayerIndex(0), handComplete(false), 
      currentHandHasChoppedPot(false), handHistory(PokerVariant::TEXAS_HOLDEM, 1), 
      currentRound(UNIFIED_PRE_FLOP) {
    // TODO: HandHistory needs to be updated to use VariantInfo instead of PokerVariant
}

void PokerGame::showGameState() const {
    table->showTable();
}

std::vector<int> PokerGame::findWinners(const std::vector<int>& eligiblePlayers) {
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
            // If player hasn't acted this round, betting isn't complete
            if (!hasActedThisRound[i]) {
                return false;
            }
            // If player's inFor amount doesn't match current bet, betting isn't complete
            if (player->getInFor() < currentBet) {
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
                int raiseAmount = player->calculateRaiseAmount(handHistory, currentBet, variantInfo);
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
                          (decision == PlayerAction::RAISE) ? player->calculateRaiseAmount(handHistory, currentBet, variantInfo) : callAmount,
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
            showGameState();
            completeBettingRound(HandHistoryRound::PRE_FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_FLOP) { // Fourth street
            std::cout << "\n=== FOURTH STREET ===" << std::endl;
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            completeBettingRound(HandHistoryRound::FLOP);
            nextRound();
        } else if (currentRound == UNIFIED_TURN) { // Fifth street
            std::cout << "\n=== FIFTH STREET ===" << std::endl;
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            completeBettingRound(HandHistoryRound::TURN);
            nextRound();
        } else if (currentRound == UNIFIED_RIVER) { // Sixth street
            std::cout << "\n=== SIXTH STREET ===" << std::endl;
            // Deal one up card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), true);
                }
            }
            showGameState();
            completeBettingRound(HandHistoryRound::RIVER);
            nextRound();
        } else if (currentRound == UNIFIED_FINAL) { // Seventh street
            std::cout << "\n=== SEVENTH STREET ===" << std::endl;
            // Deal final down card to each player
            for (int i = 0; i < table->getPlayerCount(); i++) {
                Player* player = table->getPlayer(i);
                if (player && !player->hasFolded()) {
                    player->addCard(table->getDeck().dealCard(), false);
                }
            }
            showGameState();
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
            if (static_cast<int>(upCard.getRank()) < static_cast<int>(lowestCard.getRank())) {
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
        
        std::cout << bringInPlayerPtr->getName() << " has lowest card (" 
                  << lowestCard.toString() << ") and must bring-in for $" << bringIn << std::endl;
        std::cout << bringInPlayerPtr->getName() << " brings in for $" << bringIn << std::endl;
        
        // First to act is next player after bring-in
        currentPlayerIndex = (bringInPlayer + 1) % table->getPlayerCount();
    }
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
    
    // Reset betting round state
    resetBettingRound();
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
    
    // Show all players' cards
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
    
    awardPotsStaged();
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