#include "seven_card_stud.h"
#include <iostream>
#include <algorithm>

SevenCardStud::SevenCardStud(Table* gameTable, int ante, int bringIn, int smallBet, int largeBet)
    : PokerGame(gameTable, VariantConfig(PokerVariant::SEVEN_CARD_STUD, ante, bringIn, smallBet, largeBet)),
      currentRound(StudRound::THIRD_STREET), bettingComplete(false), bringInPlayer(-1) {}

void SevenCardStud::startNewHand() {
    if (!table || table->getPlayerCount() < 2) {
        std::cout << "Need at least 2 players to start a hand!" << std::endl;
        return;
    }
    
    // Initialize hasActedThisRound vector for all players
    hasActedThisRound.assign(table->getPlayerCount(), false);
    
    table->startNewHand();
    collectAntes();
    dealInitialCards();
    // Note: bring-in determination moved to runBettingRounds()
    startBettingRound(StudRound::THIRD_STREET);
}

void SevenCardStud::dealInitialCards() {
    dealThirdStreet();
}

void SevenCardStud::runBettingRounds() {
    std::cout << "\n=== THIRD STREET ===" << std::endl;
    showStudGameState();
    
    // Determine and announce bring-in after showing cards
    determineBringInPlayer();
    
    // Set the bring-in player as the current player for third street
    currentPlayerIndex = bringInPlayer;
    
    // Handle bring-in first on third street
    if (bringInPlayer != -1 && currentPlayerIndex == bringInPlayer && !hasActedThisRound[bringInPlayer]) {
        playerBringIn(bringInPlayer);
    }
    
    // Complete all betting rounds using base class method
    while (!isHandComplete()) {
        if (currentRound == StudRound::THIRD_STREET) {
            completeBettingRound(HandHistoryRound::PRE_FLOP);
        } else if (currentRound == StudRound::FOURTH_STREET) {
            completeBettingRound(HandHistoryRound::FLOP);
        } else if (currentRound == StudRound::FIFTH_STREET) {
            completeBettingRound(HandHistoryRound::TURN);
        } else if (currentRound == StudRound::SIXTH_STREET) {
            completeBettingRound(HandHistoryRound::RIVER);
        } else if (currentRound == StudRound::SEVENTH_STREET) {
            completeBettingRound(HandHistoryRound::RIVER);
        }
        
        if (!isHandComplete()) {
            nextStreet();
        }
    }
}

void SevenCardStud::conductShowdown() {
    std::cout << "\n=== SHOWDOWN ===" << std::endl;
    
    // Show all players' complete hands
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            std::cout << player->getName() << ": ";
            player->showHand();
            std::cout << std::endl;
        }
    }
    
    table->showPotBreakdown();
    awardPotsStaged();
}

bool SevenCardStud::atShowdown() const {
    return currentRound == StudRound::SHOWDOWN;
}

void SevenCardStud::collectAntes() {
    // Each player posts an ante
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            player->addToInFor(config.ante);
            // Don't set currentBet for antes - they're not part of betting round logic
        }
    }
    // Collect the antes into the pot
    collectBetsToInFor();
}

void SevenCardStud::dealThirdStreet() {
    // Deal 2 cards face down, 1 card face up to each player
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player) {
            // Two face down cards
            player->addCard(table->getDeck().dealCard(), false);
            player->addCard(table->getDeck().dealCard(), false);
            // One face up card
            player->addCard(table->getDeck().dealCard(), true);
        }
    }
}

void SevenCardStud::dealFourthStreet() {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            player->addCard(table->getDeck().dealCard(), true);
        }
    }
}

void SevenCardStud::dealFifthStreet() {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            player->addCard(table->getDeck().dealCard(), true);
        }
    }
}

void SevenCardStud::dealSixthStreet() {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            player->addCard(table->getDeck().dealCard(), true);
        }
    }
}

void SevenCardStud::dealSeventhStreet() {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            player->addCard(table->getDeck().dealCard(), false); // Face down
        }
    }
}

void SevenCardStud::determineBringInPlayer() {
    int lowestPlayerIndex = -1;
    Card lowestCard(Suit::CLUBS, Rank::ACE); // Start with highest possible card
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            Card upCard = player->getLowestUpCard();
            if (static_cast<int>(upCard.getRank()) < static_cast<int>(lowestCard.getRank())) {
                lowestCard = upCard;
                lowestPlayerIndex = i;
            }
        }
    }
    
    bringInPlayer = lowestPlayerIndex;
    if (bringInPlayer != -1) {
        Player* player = table->getPlayer(bringInPlayer);
        std::cout << player->getName() << " has lowest card (" << lowestCard.toString() 
                  << ") and must bring-in for $" << config.bringIn << std::endl;
        
        // Set the current player to the bring-in player for third street
        if (currentRound == StudRound::THIRD_STREET) {
            currentPlayerIndex = bringInPlayer;
        }
    }
}

void SevenCardStud::startBettingRound(StudRound round) {
    currentRound = round;
    resetBettingRound(); // Use base class method
    
    if (round == StudRound::THIRD_STREET) {
        // For third street, current player is the bring-in player (set in runBettingRounds)
        // Don't set currentPlayerIndex here - it's set after determineBringInPlayer
    } else {
        // On later streets, player with highest showing hand acts first
        currentPlayerIndex = findHighestShowingHand();
    }
}

int SevenCardStud::findHighestShowingHand() const {
    int bestPlayerIndex = -1;
    HandResult bestShowing;
    bestShowing.rank = HandRank::HIGH_CARD;
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            std::vector<Card> upCards = player->getUpCards();
            if (!upCards.empty()) {
                // Evaluate just the up cards to determine betting order
                HandResult showing = HandEvaluator::evaluateHand(upCards, {});
                if (showing > bestShowing) {
                    bestShowing = showing;
                    bestPlayerIndex = i;
                }
            }
        }
    }
    return bestPlayerIndex;
}

void SevenCardStud::nextStreet() {
    if (isBettingComplete()) {
        collectBetsToInFor();
        
        switch (currentRound) {
            case StudRound::THIRD_STREET:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== FOURTH STREET ===" << std::endl;
                    dealFourthStreet();
                    showStudGameState();
                    startBettingRound(StudRound::FOURTH_STREET);
                } else {
                    currentRound = StudRound::SHOWDOWN;
                }
                break;
            case StudRound::FOURTH_STREET:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== FIFTH STREET ===" << std::endl;
                    dealFifthStreet();
                    showStudGameState();
                    startBettingRound(StudRound::FIFTH_STREET);
                } else {
                    currentRound = StudRound::SHOWDOWN;
                }
                break;
            case StudRound::FIFTH_STREET:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== SIXTH STREET ===" << std::endl;
                    dealSixthStreet();
                    showStudGameState();
                    startBettingRound(StudRound::SIXTH_STREET);
                } else {
                    currentRound = StudRound::SHOWDOWN;
                }
                break;
            case StudRound::SIXTH_STREET:
                if (countActivePlayers() > 1) {
                    std::cout << "\n=== SEVENTH STREET ===" << std::endl;
                    dealSeventhStreet();
                    std::cout << "Final cards dealt face down" << std::endl;
                    // Show current hands but without the new face-down card visible
                    std::cout << "Player hands (face-up cards shown):" << std::endl;
                    for (int i = 0; i < table->getPlayerCount(); i++) {
                        Player* player = table->getPlayer(i);
                        if (player && !player->hasFolded()) {
                            player->showStudHand();
                            std::cout << "($" << player->getChips() << ")" << std::endl;
                        }
                    }
                    table->showPotBreakdown();
                    std::cout << std::endl; // Add blank line after pot breakdown
                    startBettingRound(StudRound::SEVENTH_STREET);
                } else {
                    currentRound = StudRound::SHOWDOWN;
                }
                break;
            case StudRound::SEVENTH_STREET:
                currentRound = StudRound::SHOWDOWN;
                break;
            case StudRound::SHOWDOWN:
                break;
        }
        
        // Only conduct showdown when we actually reach the showdown round
        if (currentRound == StudRound::SHOWDOWN) {
            conductShowdown();
        }
    }
}

void SevenCardStud::showStudGameState() const {
    std::cout << "Player hands (face-up cards shown):" << std::endl;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            player->showStudHand();
            std::cout << "($" << player->getChips() << ")" << std::endl;
        }
    }
    
    // Show pot breakdown
    table->showPotBreakdown();
    std::cout << std::endl; // Add blank line after pot breakdown
}

void SevenCardStud::autoCompleteCurrentBettingRound() {
    int actionCount = 0;
    bool bringInAlreadyDone = false; // Track if bring-in has already happened
    
    while (!isBettingComplete() && actionCount < 8) {
        int playerIndex = currentPlayerIndex;
        if (playerIndex != -1 && canPlayerAct(playerIndex)) {
            if (currentRound == StudRound::THIRD_STREET && playerIndex == bringInPlayer && !bringInAlreadyDone) {
                // Bring-in player acts first on third street - but only once!
                playerBringIn(playerIndex);
                bringInAlreadyDone = true;
            } else if (!hasActedThisRound[playerIndex]) {
                // Other players must respond to the current bet
                int currentBet = table->getCurrentBet();
                Player* player = table->getPlayer(playerIndex);
                int callAmount = currentBet - player->getCurrentBet();
                
                if (callAmount > 0) {
                    // There's a bet to call
                    if (currentBet == config.bringIn && actionCount <= 2) {
                        // Early action after bring-in - sometimes complete to small bet
                        if (playerIndex % 2 == 0) { // Simple logic: even players complete sometimes
                            playerComplete(playerIndex);
                        } else {
                            studPlayerCall(playerIndex);
                        }
                    } else {
                        // Normal call or call after complete
                        studPlayerCall(playerIndex);
                    }
                } else {
                    // No bet to call, can check
                    studPlayerCheck(playerIndex);
                }
            } else {
                // Player has already acted, move to next
                advanceToNextPlayer();
            }
            actionCount++;
        } else {
            break;
        }
    }
}

// Simplified placeholder implementations
bool SevenCardStud::canPlayerAct(int playerIndex) const { 
    Player* player = table->getPlayer(playerIndex);
    return player && !player->hasFolded() && !player->isAllIn() && playerIndex == currentPlayerIndex;
}
StudRound SevenCardStud::getCurrentRound() const { return currentRound; }
int SevenCardStud::getCurrentBet() const { return table->getCurrentBet(); }

bool SevenCardStud::playerComplete(int playerIndex) {
    // Complete the bring-in to a full small bet
    Player* player = table->getPlayer(playerIndex);
    int completeAmount = config.smallBet;
    
    player->raise(completeAmount);
    table->setCurrentBet(completeAmount);
    std::cout << player->getName() << " completes to $" << completeAmount << std::endl;
    hasActedThisRound[playerIndex] = true;
    
    // Reset action for ALL other players since there's now a new bet to call
    // This includes the bring-in player who must now respond to the complete
    for (int i = 0; i < table->getPlayerCount(); i++) {
        if (i != playerIndex) {
            hasActedThisRound[i] = false;
        }
    }
    
    advanceToNextPlayer();
    return true;
}

// 7CS-specific wrappers that handle hasActedThisRound tracking
bool SevenCardStud::studPlayerCall(int playerIndex) {
    bool result = PokerGame::playerCall(playerIndex);
    if (result) {
        hasActedThisRound[playerIndex] = true;
        advanceToNextPlayer();
    }
    return result;
}

bool SevenCardStud::studPlayerCheck(int playerIndex) {
    bool result = PokerGame::playerCheck(playerIndex);
    if (result) {
        hasActedThisRound[playerIndex] = true;
        advanceToNextPlayer();
    }
    return result;
}

bool SevenCardStud::studPlayerFold(int playerIndex) {
    bool result = PokerGame::playerFold(playerIndex);
    if (result) {
        hasActedThisRound[playerIndex] = true;
        advanceToNextPlayer();
    }
    return result;
}

bool SevenCardStud::playerBringIn(int playerIndex) { 
    Player* player = table->getPlayer(playerIndex);
    
    // For now, simplified: always bring-in for the minimum amount
    // In a real game, player would choose between bring-in ($10) or complete ($20)
    std::cout << player->getName() << " brings in for $" << config.bringIn << std::endl;
    
    // Use inFor system: chips already deducted and added to inFor
    int additionalAmount = config.bringIn - player->getCurrentBet();
    player->addToInFor(additionalAmount);
    player->setBet(config.bringIn);
    table->setCurrentBet(config.bringIn);
    hasActedThisRound[playerIndex] = true;
    advanceToNextPlayer();
    return true; 
}
void SevenCardStud::advanceToNextPlayer() {
    currentPlayerIndex = getNextActivePlayer(currentPlayerIndex);
}
bool SevenCardStud::isBettingComplete() const { 
    // Simplified: betting complete when no valid current player or all have acted
    return currentPlayerIndex == -1 || allPlayersActedThisRound();
}
bool SevenCardStud::allPlayersActed() const { return true; }
bool SevenCardStud::allPlayersActedThisRound() const { 
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            if (i < static_cast<int>(hasActedThisRound.size()) && !hasActedThisRound[i]) {
                return false;
            }
        }
    }
    return true;
}
int SevenCardStud::countActivePlayers() const { 
    int count = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            count++;
        }
    }
    return count;
}
bool SevenCardStud::allRemainingPlayersAllIn() const { return false; }
bool SevenCardStud::isActionComplete() const { return false; }

int SevenCardStud::getNextActivePlayer(int startIndex) const { 
    int playerCount = table->getPlayerCount();
    for (int i = 1; i <= playerCount; i++) {
        int nextIndex = (startIndex + i) % playerCount;
        Player* player = table->getPlayer(nextIndex);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            return nextIndex;
        }
    }
    return -1; // No active players found
}
void SevenCardStud::resetBettingState() {
    for (int i = 0; i < table->getPlayerCount(); i++) {
        hasActedThisRound[i] = false;
    }
}

// Override for 7CS specifics
std::vector<int> SevenCardStud::findBestHand(const std::vector<int>& eligiblePlayers) const {
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
                {} // No community cards in 7-Card Stud
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

void SevenCardStud::displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const {
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult hand = HandEvaluator::evaluateHand(
                player->getHand(), 
                {} // No community cards in 7-Card Stud
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