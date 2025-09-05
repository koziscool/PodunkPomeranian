#include "omaha_hi_lo.h"
#include "hand_evaluator.h"
#include <iostream>
#include <algorithm>

OmahaHiLo::OmahaHiLo(Table* gameTable) 
    : PokerGame(gameTable, VariantConfig(PokerVariant::OMAHA_HI_LO, 10, 20)), 
      config(PokerVariant::OMAHA_HI_LO, 10, 20),
      currentRound(OmahaBettingRound::PRE_FLOP), bettingComplete(false), currentPlayerIndex(0) {
}

void OmahaHiLo::startNewHand() {
    currentRound = OmahaBettingRound::PRE_FLOP;
    bettingComplete = false;
    hasActedThisRound.assign(table->getPlayerCount(), false);
    
    // Deal 4 hole cards to each player
    dealInitialCards();
    
    // Post blinds
    postBlinds();
    
    // Set initial player to act (UTG - 3 seats after dealer, which wraps to Alice=0)
    int dealerPos = table->getDealerPosition(); // Bob = 1
    int playerCount = table->getPlayerCount();
    currentPlayerIndex = (dealerPos + 3) % playerCount; // (1+3)%4 = 0 = Alice
}

void OmahaHiLo::dealInitialCards() {
    // Deal 4 hole cards to each player (face down)
    for (int round = 0; round < 4; round++) {
        for (int i = 0; i < table->getPlayerCount(); i++) {
            Player* player = table->getPlayer(i);
            if (player) {
                Card card = table->getDeck().dealCard();
                player->addCard(card); // Face down by default
            }
        }
    }
}

void OmahaHiLo::postBlinds() {
    int dealerPos = table->getDealerPosition(); // Bob = 1
    int playerCount = table->getPlayerCount();
    
    // Small blind (next after dealer): Charlie = 2  
    int smallBlindPos = (dealerPos + 1) % playerCount;
    Player* smallBlindPlayer = table->getPlayer(smallBlindPos);
    
    // Big blind (next after small blind): Diana = 3
    int bigBlindPos = (dealerPos + 2) % playerCount;
    Player* bigBlindPlayer = table->getPlayer(bigBlindPos);
    
    if (smallBlindPlayer && bigBlindPlayer) {
        smallBlindPlayer->addToInFor(config.smallBlind);
        table->setCurrentBet(config.smallBlind);
        bigBlindPlayer->addToInFor(config.bigBlind);
        table->setCurrentBet(config.bigBlind);
        std::cout << smallBlindPlayer->getName() << " posts SB $" << config.smallBlind 
                  << ", " << bigBlindPlayer->getName() << " posts BB $" << config.bigBlind << std::endl;
        
        // Blinds are forced bets, not actions on their hand
        // Nobody has acted on their hand yet
        // All players (including blind players) still need to act
    }
}

void OmahaHiLo::runBettingRounds() {
    while (!isHandComplete()) {
        if (currentRound == OmahaBettingRound::PRE_FLOP) {
            std::cout << "\n=== PRE-FLOP ===" << std::endl;
            showGameState();
            autoCompleteCurrentBettingRound();
            nextPhase();
        } else if (currentRound == OmahaBettingRound::FLOP) {
            std::cout << "\n=== FLOP ===" << std::endl;
            table->dealFlop();
            showGameState();
            autoCompleteCurrentBettingRound();
            nextPhase();
        } else if (currentRound == OmahaBettingRound::TURN) {
            std::cout << "\n=== TURN ===" << std::endl;
            table->dealTurn();
            showGameState();
            autoCompleteCurrentBettingRound();
            nextPhase();
        } else if (currentRound == OmahaBettingRound::RIVER) {
            std::cout << "\n=== RIVER ===" << std::endl;
            table->dealRiver();
            showGameState();
            autoCompleteCurrentBettingRound();
            currentRound = OmahaBettingRound::SHOWDOWN;
        }
    }
    
    // Conduct showdown if we reached it
    if (currentRound == OmahaBettingRound::SHOWDOWN) {
        conductShowdown();
    }
}

void OmahaHiLo::showOmahaGameState() {
    // Show community cards if any
    if (!table->getCommunityCards().empty()) {
        std::cout << "Community Cards: ";
        for (const auto& card : table->getCommunityCards()) {
            std::cout << card.toString() << " ";
        }
        std::cout << std::endl;
    }
    
    // Show pot breakdown
    table->getSidePotManager().showPotBreakdown();
    std::cout << std::endl;
}

void OmahaHiLo::autoCompleteCurrentBettingRound() {
    // Basic auto-complete - everyone calls or checks (same as Hold'em)
    int actionCount = 0;
    while (!isBettingComplete() && actionCount < 10) { // Increased limit for safety
        int playerIndex = currentPlayerIndex;
        
        if (playerIndex != -1 && canPlayerAct(playerIndex)) {
            Player* player = table->getPlayer(playerIndex);
            int currentBet = table->getCurrentBet();
            
            if (currentBet > 0) {
                if (player->getInFor() < currentBet) {
                    // Player needs to call (put in more money)
                    int callAmount = currentBet - player->getInFor();
                    if (callAmount >= player->getChips() && player->getChips() <= 200) {
                        playerFold(playerIndex);
                    } else {
                        playerCall(playerIndex);
                    }
                } else {
                    // Player is already matched - can check or raise (blind option)
                    // Let's have Diana (big blind) raise to test the raise logic
                    if (playerIndex == 3) { // Diana
                        playerRaise(playerIndex, currentBet * 2); // Raise to 2x current bet
                    } else {
                        playerCheck(playerIndex);
                    }
                }
            } else {
                // No current bet - everyone can check
                playerCheck(playerIndex);
            }
            
            // Mark player as having acted on their hand
            hasActedThisRound[playerIndex] = true;
            
            // Advance to next player
            advanceToNextPlayer();
            actionCount++;
        } else {
            break;
        }
    }
    
    // Collect the inFor amounts to pots
    collectBetsToInFor();
}

void OmahaHiLo::nextPhase() {
    if (currentRound == OmahaBettingRound::PRE_FLOP) {
        currentRound = OmahaBettingRound::FLOP;
    } else if (currentRound == OmahaBettingRound::FLOP) {
        currentRound = OmahaBettingRound::TURN;
    } else if (currentRound == OmahaBettingRound::TURN) {
        currentRound = OmahaBettingRound::RIVER;
    } else if (currentRound == OmahaBettingRound::RIVER) {
        currentRound = OmahaBettingRound::SHOWDOWN;
    }
}

void OmahaHiLo::conductShowdown() {
    std::cout << "\n=== SHOWDOWN ===" << std::endl;
    
    // Show all players' hole cards
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
    
    // Debug: Check if there are any pots
    const auto& pots = table->getSidePotManager().getPots();
    std::cout << "Number of pots: " << pots.size() << std::endl;
    for (size_t i = 0; i < pots.size(); i++) {
        std::cout << "Pot " << i << ": $" << pots[i].amount << " with " << pots[i].eligiblePlayers.size() << " eligible players" << std::endl;
    }
    
    awardPotsStaged();
}

bool OmahaHiLo::isHandComplete() const {
    return currentRound == OmahaBettingRound::SHOWDOWN;
}

void OmahaHiLo::transferPotToWinners(int potAmount, const std::vector<int>& winners) {
    if (winners.empty()) {
        std::cout << "No winners found for pot!" << std::endl;
        return;
    }
    
    // Find best low hands among all eligible players (not just high winners)
    std::vector<int> lowWinners;
    HandResult bestLow;
    bestLow.values = {15, 15, 15, 15, 15}; // Invalid low marker
    bool foundQualifyingLow = false;
    
    // Check all players for qualifying low hands
    for (int playerIndex : winners) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            HandResult playerLow = evaluateOmahaLowHand(player->getHand(), table->getCommunityCards());
            
            // Check if this is a valid qualifying low
            if (playerLow.values[0] != 15) { // Not the invalid marker
                if (!foundQualifyingLow || playerLow < bestLow) {
                    bestLow = playerLow;
                    lowWinners.clear();
                    lowWinners.push_back(playerIndex);
                    foundQualifyingLow = true;
                } else if (foundQualifyingLow && playerLow == bestLow) {
                    lowWinners.push_back(playerIndex);
                }
            }
        }
    }
    
    // Split pot between high and low, or award all to high if no qualifying low
    if (foundQualifyingLow) {
        int halfPot = potAmount / 2;
        int remainder = potAmount % 2;
        
        // Award high half (plus any remainder)
        std::cout << "High hand wins $" << (halfPot + remainder) << ": ";
        for (size_t i = 0; i < winners.size(); i++) {
            Player* winner = table->getPlayer(winners[i]);
            int award = (halfPot + remainder) / static_cast<int>(winners.size());
            winner->addChips(award);
            std::cout << winner->getName() << " (+$" << award << ")";
            if (i < winners.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Award low half
        std::cout << "Low hand wins $" << halfPot << ": ";
        for (size_t i = 0; i < lowWinners.size(); i++) {
            Player* winner = table->getPlayer(lowWinners[i]);
            int award = halfPot / static_cast<int>(lowWinners.size());
            winner->addChips(award);
            std::cout << winner->getName() << " (+$" << award << ")";
            if (i < lowWinners.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    } else {
        // No qualifying low - high hand gets entire pot
        std::cout << "High hand wins entire pot $" << potAmount << " (no qualifying low): ";
        for (size_t i = 0; i < winners.size(); i++) {
            Player* winner = table->getPlayer(winners[i]);
            int award = potAmount / static_cast<int>(winners.size());
            winner->addChips(award);
            std::cout << winner->getName() << " (+$" << award << ")";
            if (i < winners.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    if (winners.size() > 1 || lowWinners.size() > 1) {
        currentHandHasChoppedPot = true;
    }
}

std::vector<int> OmahaHiLo::findBestHand(const std::vector<int>& eligiblePlayers) const {
    if (eligiblePlayers.empty()) {
        return {};
    }
    
    HandResult bestHand;
    bestHand.rank = HandRank::HIGH_CARD;
    std::vector<int> winners;
    
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            // Enforce Omaha rule: exactly 2 hole cards + 3 community cards
            HandResult bestPlayerHand = evaluateOmahaHand(player->getHand(), table->getCommunityCards());
            
            if (bestPlayerHand > bestHand) {
                bestHand = bestPlayerHand;
                winners.clear();
                winners.push_back(playerIndex);
            } else if (bestPlayerHand == bestHand) {
                winners.push_back(playerIndex);
            }
        }
    }
    
    return winners;
}

std::vector<int> OmahaHiLo::findHighWinners(const std::vector<int>& eligiblePlayers) const {
    if (eligiblePlayers.empty()) return {};
    
    HandResult bestHigh;
    bestHigh.rank = HandRank::HIGH_CARD;
    std::vector<int> winners;
    
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            // For now, use basic hand evaluation - will need Omaha-specific evaluation
            HandResult hand = HandEvaluator::evaluateHand(
                player->getHand(), 
                table->getCommunityCards()
            );
            
            if (hand > bestHigh) {
                bestHigh = hand;
                winners.clear();
                winners.push_back(playerIndex);
            } else if (hand == bestHigh) {
                winners.push_back(playerIndex);
            }
        }
    }
    
    return winners;
}

std::vector<int> OmahaHiLo::findLowWinners(const std::vector<int>& eligiblePlayers) const {
    // Placeholder for low hand evaluation
    // For now, return empty (no qualifying low)
    std::vector<std::pair<int, HandResult>> playersWithLow;
    
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            if (qualifiesForLow(player->getHand(), table->getCommunityCards())) {
                // Would evaluate low hand here
                // For now, skip low evaluation
            }
        }
    }
    
    std::vector<int> lowWinners;
    if (!playersWithLow.empty()) {
        // Find best low hand
        auto bestPlayerLow = *std::min_element(playersWithLow.begin(), playersWithLow.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Find all players tied for best low
        for (const auto& playerLow : playersWithLow) {
            if (playerLow.second == bestPlayerLow.second) {
                lowWinners.clear();
                lowWinners.push_back(playerLow.first);
            } else if (playerLow.second == bestPlayerLow.second) {
                lowWinners.push_back(playerLow.first);
            }
        }
    }
    
    return lowWinners;
}

bool OmahaHiLo::qualifiesForLow(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const {
    (void)holeCards;
    (void)communityCards;
    // Placeholder - for now return false (no qualifying low)
    return false;
}

void OmahaHiLo::displayWinningHands(const std::vector<int>& winners, const std::vector<int>& eligiblePlayers) const {
    std::cout << "\n=== HAND ANALYSIS ===\n";
    
    // Show each player's hand (but note: evaluation is incorrect for Omaha)
    for (int playerIndex : eligiblePlayers) {
        Player* player = table->getPlayer(playerIndex);
        if (player && !player->hasFolded()) {
            std::cout << player->getName() << ": ";
            
            // Mark actual winners
            bool isWinner = std::find(winners.begin(), winners.end(), playerIndex) != winners.end();
            if (isWinner) std::cout << "WINNER ";
            
            // Show hand evaluation (NOTE: This is wrong for Omaha - doesn't use 2+3 rule)
            HandResult hand = HandEvaluator::evaluateHand(player->getHand(), table->getCommunityCards());
            std::cout << hand.description << " (WARNING: Omaha 2+3 rule not implemented)";
            
            std::cout << std::endl;
        }
    }
}

// Betting logic methods (same as Hold'em)
bool OmahaHiLo::canPlayerAct(int playerIndex) const {
    Player* player = table->getPlayer(playerIndex);
    return player && !player->hasFolded() && !player->isAllIn() && playerIndex == currentPlayerIndex;
}

bool OmahaHiLo::isBettingComplete() const {
    // Check if all active players have acted and all bets are matched
    int currentBet = table->getCurrentBet();
    
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            // If player hasn't acted this round and isn't all-in, betting isn't complete
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

int OmahaHiLo::countActivePlayers() const {
    int count = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded()) {
            count++;
        }
    }
    return count;
}

bool OmahaHiLo::allRemainingPlayersAllIn() const {
    int activeNonAllInCount = 0;
    for (int i = 0; i < table->getPlayerCount(); i++) {
        Player* player = table->getPlayer(i);
        if (player && !player->hasFolded() && !player->isAllIn()) {
            activeNonAllInCount++;
        }
    }
    return activeNonAllInCount <= 1;
}

bool OmahaHiLo::isActionComplete() const {
    return allRemainingPlayersAllIn();
}

HandResult OmahaHiLo::evaluateOmahaHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const {
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
                        }
                    }
                }
            }
        }
    }
    
    return bestHand;
}

HandResult OmahaHiLo::evaluateOmahaLowHand(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) const {
    HandResult bestLow;
    bestLow.rank = HandRank::HIGH_CARD;
    bestLow.values = {14, 14, 14, 14, 14}; // Worst possible low (higher = worse for low)
    bool foundQualifyingLow = false;
    
    if (holeCards.size() < 2 || communityCards.size() < 3) {
        return bestLow; // Not enough cards
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
                        
                        // Check if this qualifies for low (8-or-better)
                        if (hasQualifyingLow(fiveCards)) {
                            // Evaluate as low hand (ace=1, lower is better)
                            HandResult lowResult = evaluateLowHand(fiveCards);
                            
                            if (!foundQualifyingLow || lowResult < bestLow) {
                                bestLow = lowResult;
                                foundQualifyingLow = true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (!foundQualifyingLow) {
        bestLow.rank = HandRank::HIGH_CARD;
        bestLow.values = {15, 15, 15, 15, 15}; // Invalid low
    }
    
    return bestLow;
}

bool OmahaHiLo::hasQualifyingLow(const std::vector<Card>& fiveCards) const {
    // Check if all cards are 8 or lower (ace counts as 1 for low)
    for (const Card& card : fiveCards) {
        int lowValue = (card.getRank() == Rank::ACE) ? 1 : static_cast<int>(card.getRank());
        if (lowValue > 8) {
            return false;
        }
    }
    return true;
}

HandResult OmahaHiLo::evaluateLowHand(const std::vector<Card>& fiveCards) const {
    HandResult result;
    result.rank = HandRank::HIGH_CARD; // Low hands are always "high card" rank
    
    // Convert to low values (A=1, 2-8 = face value)
    std::vector<int> lowValues;
    for (const Card& card : fiveCards) {
        int lowValue = (card.getRank() == Rank::ACE) ? 1 : static_cast<int>(card.getRank());
        lowValues.push_back(lowValue);
    }
    
    // Sort ascending for low (lowest first)
    std::sort(lowValues.begin(), lowValues.end());
    
    result.values = lowValues;
    return result;
}

void OmahaHiLo::advanceToNextPlayer() {
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