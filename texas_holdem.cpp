#include "texas_holdem.h"
#include <iostream>

TexasHoldem::TexasHoldem(Table* gameTable, int smallBlind, int bigBlind)
    : PokerGame(gameTable, VariantConfig(PokerVariant::TEXAS_HOLDEM, smallBlind, bigBlind)),
      currentRound(BettingRound::PRE_FLOP), bettingComplete(false) {}

void TexasHoldem::startNewHand() {
    if (!table || table->getPlayerCount() < 2) {
        std::cout << "Need at least 2 players to start a hand!" << std::endl;
        return;
    }
    
    // Initialize hasActedThisRound vector for all players
    hasActedThisRound.assign(table->getPlayerCount(), false);
    
    table->startNewHand();
    dealInitialCards();
    postBlinds();
    startBettingRound(BettingRound::PRE_FLOP);
}

void TexasHoldem::dealInitialCards() {
    table->dealHoleCards();
}

void TexasHoldem::runBettingRounds() {
    std::cout << "\n=== PRE-FLOP ===" << std::endl;
    
    // Complete pre-flop betting 
    while (!isHandComplete() && currentRound == BettingRound::PRE_FLOP) {
        if (currentPlayerIndex != -1) {
            autoCompleteCurrentBettingRound();
        }
        if (currentRound == BettingRound::PRE_FLOP) {
            nextPhase();
        }
    }
    
    // Complete the rest of the hand
    while (!isHandComplete()) {
        if (!isHandComplete() && currentPlayerIndex != -1) {
            autoCompleteCurrentBettingRound();
        }
        if (!isHandComplete()) {
            nextPhase();
        }
    }
}

void TexasHoldem::conductShowdown() {
    table->showTable();
    table->showPotBreakdown();
    awardPotsStaged();
}

bool TexasHoldem::isHandComplete() const {
    return currentRound == BettingRound::SHOWDOWN || countActivePlayers() <= 1;
}

void TexasHoldem::postBlinds() {
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    
    // Small blind (next after dealer)
    int smallBlindPos = (dealerPos + 1) % playerCount;
    Player* smallBlindPlayer = table->getPlayer(smallBlindPos);
    
    // Big blind (next after small blind)
    int bigBlindPos = (dealerPos + 2) % playerCount;
    Player* bigBlindPlayer = table->getPlayer(bigBlindPos);
    
    if (smallBlindPlayer && bigBlindPlayer) {
        smallBlindPlayer->call(config.smallBlind);
        table->setCurrentBet(config.smallBlind);
        bigBlindPlayer->call(config.bigBlind);
        table->setCurrentBet(config.bigBlind);
        std::cout << smallBlindPlayer->getName() << " posts SB $" << config.smallBlind 
                  << ", " << bigBlindPlayer->getName() << " posts BB $" << config.bigBlind << std::endl;
    }
}

void TexasHoldem::startBettingRound(BettingRound round) {
    currentRound = round;
    resetBettingState();
    
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    
    hasActedThisRound.assign(playerCount, false);
    
    if (round == BettingRound::PRE_FLOP) {
        currentPlayerIndex = (dealerPos + 3) % playerCount;
    } else {
        currentPlayerIndex = (dealerPos + 1) % playerCount;
    }
    
    currentPlayerIndex = getNextActivePlayer(currentPlayerIndex - 1);
}

void TexasHoldem::nextPhase() {
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

// Simplified implementations for the basic functionality
void TexasHoldem::autoCompleteCurrentBettingRound() {
    // Basic auto-complete - everyone calls or checks
    int actionCount = 0;
    while (!isBettingComplete() && actionCount < 4) {
        int playerIndex = currentPlayerIndex;
        if (playerIndex != -1 && canPlayerAct(playerIndex)) {
            if (table->getCurrentBet() > 0) {
                Player* player = table->getPlayer(playerIndex);
                if (player->getCurrentBet() < table->getCurrentBet()) {
                    int callAmount = table->getCurrentBet() - player->getCurrentBet();
                    if (callAmount >= player->getChips() && player->getChips() <= 200) {
                        playerFold(playerIndex);
                    } else {
                        playerCall(playerIndex);
                    }
                } else {
                    playerCheck(playerIndex);
                }
            } else {
                playerCheck(playerIndex);
            }
            actionCount++;
        } else {
            break;
        }
    }
}

bool TexasHoldem::canPlayerAct(int playerIndex) const {
    Player* player = table->getPlayer(playerIndex);
    return player && !player->hasFolded() && !player->isAllIn() && playerIndex == currentPlayerIndex;
}

BettingRound TexasHoldem::getCurrentRound() const {
    return currentRound;
}

// Placeholder implementations for other methods - we can implement these fully later
void TexasHoldem::advanceToNextPlayer() {}
bool TexasHoldem::isBettingComplete() const { return true; }
bool TexasHoldem::allPlayersActed() const { return true; }
bool TexasHoldem::allPlayersActedThisRound() const { return true; }
int TexasHoldem::countActivePlayers() const { return table->getPlayerCount(); }
bool TexasHoldem::allRemainingPlayersAllIn() const { return false; }
bool TexasHoldem::isActionComplete() const { return false; }
int TexasHoldem::getNextActivePlayer(int startIndex) const { (void)startIndex; return 0; }
void TexasHoldem::resetBettingState() {} 