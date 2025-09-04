#include "game.h"
#include <iostream>
#include <set>
#include <map>
#include <algorithm>

Game::Game(Table* gameTable, int smallBlind, int bigBlind)
    : table(gameTable), currentRound(BettingRound::PRE_FLOP), 
      currentPlayerIndex(0), bettingComplete(false),
      smallBlindAmount(smallBlind), bigBlindAmount(bigBlind), 
      currentHandHasChoppedPot(false) {}

void Game::startNewHand() {
    if (!table || table->getPlayerCount() < 2) {
        std::cout << "Need at least 2 players to start a hand!" << std::endl;
        return;
    }
    
    currentHandHasChoppedPot = false; // Reset for new hand
    
    table->startNewHand();
    
    // Deal hole cards
    table->dealHoleCards();
    
    // Post blinds
    postBlinds();
    
    // Start pre-flop betting
    startBettingRound(BettingRound::PRE_FLOP);
}

void Game::postBlinds() {
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    
    // Small blind (next after dealer)
    int smallBlindPos = (dealerPos + 1) % playerCount;
    Player* smallBlindPlayer = table->getPlayer(smallBlindPos);
    
    // Big blind (next after small blind)
    int bigBlindPos = (dealerPos + 2) % playerCount;
    Player* bigBlindPlayer = table->getPlayer(bigBlindPos);
    
    if (smallBlindPlayer && bigBlindPlayer) {
        smallBlindPlayer->call(smallBlindAmount);
        table->setCurrentBet(smallBlindAmount);
        bigBlindPlayer->call(bigBlindAmount);
        table->setCurrentBet(bigBlindAmount);
        std::cout << smallBlindPlayer->getName() << " posts SB $" << smallBlindAmount 
                  << ", " << bigBlindPlayer->getName() << " posts BB $" << bigBlindAmount << std::endl;
    }
}

void Game::startBettingRound(BettingRound round) {
    currentRound = round;
    resetBettingState();
    
    // Determine first player to act
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    
    // Reset action tracking for this round
    hasActedThisRound.assign(playerCount, false);
    
    // All-in players have automatically "acted" for all future rounds
    for (int i = 0; i < playerCount; i++) {
        Player* player = table->getPlayer(i);
        if (player && player->isAllIn()) {
            hasActedThisRound[i] = true;
        }
    }
    
    if (round == BettingRound::PRE_FLOP) {
        // Pre-flop: first to act is after big blind
        currentPlayerIndex = (dealerPos + 3) % playerCount;
    } else {
        // Post-flop: first to act is after dealer (small blind position)
        currentPlayerIndex = (dealerPos + 1) % playerCount;
    }
    
    // Find first active player
    currentPlayerIndex = getNextActivePlayer(currentPlayerIndex - 1);
}

void Game::nextPhase() {
    if (isBettingComplete()) {
        table->collectBets();
        
        switch (currentRound) {
            case BettingRound::PRE_FLOP:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== FLOP ===" << std::endl;
                    table->dealFlop();
                    if (!allRemainingPlayersAllIn()) {
                        if (isActionComplete()) {
                            std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        }
                        startBettingRound(BettingRound::FLOP);
                    } else {
                        // Everyone all-in, deal all remaining cards and go to showdown
                        std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        std::cout << "\n=== TURN ===" << std::endl;
                        table->dealTurn();
                        std::cout << "\n=== RIVER ===" << std::endl;
                        table->dealRiver();
                        currentRound = BettingRound::SHOWDOWN;
                    }
                } else {
                    currentRound = BettingRound::SHOWDOWN;
                }
                break;
            case BettingRound::FLOP:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== TURN ===" << std::endl;
                    table->dealTurn();
                    if (!allRemainingPlayersAllIn()) {
                        if (isActionComplete()) {
                            std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        }
                        startBettingRound(BettingRound::TURN);
                    } else {
                        // Everyone all-in, deal river and go to showdown
                        std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        std::cout << "\n=== RIVER ===" << std::endl;
                        table->dealRiver();
                        currentRound = BettingRound::SHOWDOWN;
                    }
                } else {
                    currentRound = BettingRound::SHOWDOWN;
                }
                break;
            case BettingRound::TURN:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== RIVER ===" << std::endl;
                    table->dealRiver();
                    if (!allRemainingPlayersAllIn()) {
                        if (isActionComplete()) {
                            std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        }
                        startBettingRound(BettingRound::RIVER);
                    } else {
                        // Everyone all-in, skip betting and go to showdown
                        std::cout << "ACTION COMPLETE - NO FURTHER BETTING" << std::endl;
                        currentRound = BettingRound::SHOWDOWN;
                    }
                } else {
                    currentRound = BettingRound::SHOWDOWN;
                }
                break;
            case BettingRound::RIVER:
                currentRound = BettingRound::SHOWDOWN;
                break;
            case BettingRound::SHOWDOWN:
                break;
        }
        
        if (currentRound == BettingRound::SHOWDOWN) {
            conductShowdown();
        }
    }
}

bool Game::playerFold(int playerIndex) {
    if (!canPlayerAct(playerIndex)) {
        return false;
    }
    
    Player* player = table->getPlayer(playerIndex);
    player->fold();
    
    std::cout << player->getName() << " folds" << std::endl;
    
    advanceToNextPlayer();
    return true;
}

bool Game::playerCheck(int playerIndex) {
    if (!canPlayerAct(playerIndex)) {
        return false;
    }
    
    // In pre-flop, big blind can check their option even if there's a current bet (their own blind)
    Player* player = table->getPlayer(playerIndex);
    if (table->getCurrentBet() > 0 && 
        !(currentRound == BettingRound::PRE_FLOP && player->getCurrentBet() == table->getCurrentBet())) {
        return false;
    }
    
    player->check();
    
    std::cout << player->getName() << " checks" << std::endl;
    
    // Mark this player as having acted this round
    hasActedThisRound[playerIndex] = true;
    
    advanceToNextPlayer();
    
    // If everyone has checked around, mark betting complete
    if (table->getCurrentBet() == 0 && allPlayersActedThisRound()) {
        bettingComplete = true;
    }
    
    return true;
}

bool Game::playerCall(int playerIndex) {
    if (!canPlayerAct(playerIndex)) {
        return false;
    }
    
    Player* player = table->getPlayer(playerIndex);
    int callAmount = table->getCurrentBet();
    
    PlayerAction action = player->call(callAmount);
    
    if (action == PlayerAction::ALL_IN) {
        std::cout << player->getName() << " calls all-in for $" << player->getCurrentBet() << std::endl;
    } else {
        std::cout << player->getName() << " calls $" << callAmount << std::endl;
    }
    
    // Mark this player as having acted this round
    hasActedThisRound[playerIndex] = true;
    
    advanceToNextPlayer();
    return true;
}

bool Game::playerRaise(int playerIndex, int amount) {
    if (!canPlayerAct(playerIndex) || !isValidAction(playerIndex, PlayerAction::RAISE, amount)) {
        return false;
    }
    
    Player* player = table->getPlayer(playerIndex);
    PlayerAction action = player->raise(amount);
    
    if (action == PlayerAction::ALL_IN) {
        std::cout << player->getName() << " raises all-in to $" << player->getCurrentBet() << std::endl;
        if (player->getCurrentBet() > table->getCurrentBet()) {
            table->setCurrentBet(player->getCurrentBet());
        }
    } else {
        std::cout << player->getName() << " raises to $" << amount << std::endl;
        table->setCurrentBet(amount);
    }
    
    // Mark this player as having acted this round
    hasActedThisRound[playerIndex] = true;
    
    advanceToNextPlayer();
    return true;
}

bool Game::playerAllIn(int playerIndex) {
    if (!canPlayerAct(playerIndex)) {
        return false;
    }
    
    Player* player = table->getPlayer(playerIndex);
    player->goAllIn();
    
    std::cout << player->getName() << " goes all-in for $" << player->getCurrentBet() << std::endl;
    
    if (player->getCurrentBet() > table->getCurrentBet()) {
        table->setCurrentBet(player->getCurrentBet());
    }
    
    // Mark this player as having acted this round
    hasActedThisRound[playerIndex] = true;
    
    advanceToNextPlayer();
    return true;
}

bool Game::canPlayerAct(int playerIndex) const {
    Player* player = table->getPlayer(playerIndex);
    return player && !player->hasFolded() && !player->isAllIn() && playerIndex == currentPlayerIndex;
}

bool Game::isValidAction(int playerIndex, PlayerAction action, int amount) const {
    Player* player = table->getPlayer(playerIndex);
    if (!player || !canPlayerAct(playerIndex)) {
        return false;
    }
    
    switch (action) {
        case PlayerAction::FOLD:
            return true;
        case PlayerAction::CHECK:
            return table->getCurrentBet() == 0 || player->getCurrentBet() == table->getCurrentBet();
        case PlayerAction::CALL:
            return table->getCurrentBet() > player->getCurrentBet();
        case PlayerAction::RAISE:
            if (allOtherPlayersAllIn(playerIndex)) { return false; }
            return amount >= getMinimumRaise() && player->canAfford(amount - player->getCurrentBet());
        case PlayerAction::ALL_IN:
            return true;
    }
    return false;
}

int Game::getCurrentPlayerIndex() const {
    return currentPlayerIndex;
}

BettingRound Game::getCurrentRound() const {
    return currentRound;
}

void Game::advanceToNextPlayer() {
    currentPlayerIndex = getNextActivePlayer(currentPlayerIndex);
    
    if (currentPlayerIndex == -1) {
        bettingComplete = true;
    }
}

bool Game::isBettingComplete() const {
    if (countActivePlayers() <= 1) { 
        return true; 
    }
    if (allRemainingPlayersAllIn()) { 
        return true; 
    }
    if (bettingComplete) { 
        return true; 
    }
    
    // Betting is complete when:
    // 1. Everyone has acted this round AND
    // 2. No outstanding bets (everyone has matched current bet OR is all-in for less)
    bool allMatched = allPlayersActed();  // No outstanding bets to call
    bool allActedThisRound = allPlayersActedThisRound();  // Everyone has acted
    
    return allMatched && allActedThisRound;
}

int Game::getMinimumRaise() const {
    return table->getCurrentBet() + bigBlindAmount;
}

bool Game::allPlayersActed() const {
    int currentBet = table->getCurrentBet();
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            if (player->getCurrentBet() < currentBet) {
                return false;
            }
        }
    }
    return true;
}

bool Game::allPlayersActedThisRound() const {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            if (!hasActedThisRound[i]) {
                return false;
            }
        }
    }
    return true;
}

int Game::countActivePlayers() const {
    int count = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            count++;
        }
    }
    return count;
}

bool Game::allRemainingPlayersAllIn() const {
    int activeNonAllIn = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            activeNonAllIn++;
        }
    }
    // Betting is only complete when NO players can act further (all are all-in or folded)
    return activeNonAllIn == 0;
}

bool Game::isActionComplete() const {
    // Action is complete when everyone but one player is all-in or folded
    // AND there are no outstanding bets to resolve
    int activeNonAllIn = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            activeNonAllIn++;
        }
    }
    return activeNonAllIn <= 1 && allPlayersActed();
}

bool Game::allOtherPlayersAllIn(int excludePlayerIndex) const {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        if (i == excludePlayerIndex) continue;
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            return false;
        }
    }
    return true;
}

void Game::conductShowdown() {
    table->showTable();
    table->showPotBreakdown();
    
    // Award pots with staged hand reveals
    awardPotsStaged();
}

void Game::awardPotsStaged() {
    const auto& pots = table->getSidePotManager().getPots();
    if (pots.empty()) {
        std::cout << "No pots to award!" << std::endl;
        return;
    }
    
    std::set<int> alreadyRevealed;
    std::vector<int> currentBestHands; // Track the current best hand holder(s)
    
    // Award pots from side pots to main pot (reverse order)
    for (int potIndex = static_cast<int>(pots.size()) - 1; potIndex >= 0; potIndex--) {
        const auto& pot = pots[potIndex];
        
        std::cout << "\n--- Awarding " << (potIndex == 0 ? "Main Pot" : ("Side Pot " + std::to_string(potIndex)))
                  << ": $" << pot.amount << " ---" << std::endl;
        
        // Convert eligible players set to vector
        std::vector<int> eligiblePlayers(pot.eligiblePlayers.begin(), pot.eligiblePlayers.end());
        
        // For the first pot (highest side pot), show all eligible players
        if (currentBestHands.empty()) {
            for (int playerIndex : eligiblePlayers) {
                Player* player = table->getPlayer(playerIndex);
                if (player && !player->hasFolded()) {
                    HandResult result = HandEvaluator::evaluateHand(
                        player->getHand(), 
                        table->getCommunityCards()
                    );
                    std::cout << player->getName() << ": " << result.description << std::endl;
                    alreadyRevealed.insert(playerIndex);
                }
            }
        } else {
            // For subsequent pots, show current winners and new contenders
            // But don't show players who have already lost
            for (int playerIndex : eligiblePlayers) {
                Player* player = table->getPlayer(playerIndex);
                if (player && !player->hasFolded()) {
                    // Show if this player is a current winner OR hasn't been revealed yet
                    bool isCurrentWinner = std::find(currentBestHands.begin(), currentBestHands.end(), playerIndex) != currentBestHands.end();
                    bool notYetRevealed = alreadyRevealed.find(playerIndex) == alreadyRevealed.end();
                    
                    if (isCurrentWinner || notYetRevealed) {
                        HandResult result = HandEvaluator::evaluateHand(
                            player->getHand(), 
                            table->getCommunityCards()
                        );
                        std::cout << player->getName() << ": " << result.description << std::endl;
                        alreadyRevealed.insert(playerIndex);
                    }
                }
            }
        }
        
        // Find winners and award pot
        std::vector<int> winners = findWinners(eligiblePlayers);
        awardSinglePot(pot.amount, winners);
        
        // Update the current best hands to be the winners of this pot
        currentBestHands = winners;
    }
}

void Game::awardSinglePot(int potAmount, const std::vector<int>& winners) {
    if (winners.empty()) {
        std::cout << "No winners found for pot!" << std::endl;
        return;
    }
    
    // Detect chopped pot (multiple winners)
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

std::vector<int> Game::findWinners(const std::vector<int>& eligiblePlayers) const {
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

void Game::showGameState() const {
    table->showTable();
}

bool Game::isHandComplete() const {
    return currentRound == BettingRound::SHOWDOWN || countActivePlayers() <= 1;
}

int Game::getNextActivePlayer(int startIndex) const {
    return table->getNextActivePlayer(startIndex);
}

void Game::resetBettingState() {
    bettingComplete = false;
}

void Game::autoCompleteCurrentBettingRound() {
    // Check if action is already complete
    if (isActionComplete()) {
        bettingComplete = true;
        return;
    }
    
    // Auto-complete betting for all players until betting round is complete
    int actionCount = 0;
    
    while (!isBettingComplete() && actionCount < 4) {  // Reduced from 8 to 4
        int playerIndex = getCurrentPlayerIndex();
        
        if (playerIndex != -1 && canPlayerAct(playerIndex)) {
            // If there's a bet to call, decide whether to call or fold
            if (table->getCurrentBet() > 0) {
                Player* player = table->getPlayer(playerIndex);
                if (player->getCurrentBet() < table->getCurrentBet()) {
                    // Need to call - but check if it's a huge all-in relative to stack
                    int callAmount = table->getCurrentBet() - player->getCurrentBet();
                    if (callAmount >= player->getChips()) {
                        // This would be an all-in call - fold if stack is small
                        if (player->getChips() <= 200) {
                            playerFold(playerIndex);
                        } else {
                            playerCall(playerIndex);
                        }
                    } else {
                        playerCall(playerIndex);
                    }
                } else {
                    // Can check
                    playerCheck(playerIndex);
                }
            } else {
                // No bet, just check
                playerCheck(playerIndex);
            }
            actionCount++;
        } else {
            break;
        }
    }
}

void Game::forceCompleteBettingRound() {
    // Force the current betting round to be marked as complete
    bettingComplete = true;
}

bool Game::hasSidePots() const {
    const auto& pots = table->getSidePotManager().getPots();
    return pots.size() > 1;
}

bool Game::hasChoppedPots() const {
    return currentHandHasChoppedPot;
}

bool Game::isInterestingHand() const {
    return hasSidePots() || hasChoppedPots();
} 