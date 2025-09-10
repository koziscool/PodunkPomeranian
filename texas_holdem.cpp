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
    
    currentRound = BettingRound::PRE_FLOP;
    bettingComplete = false;
    
    // Use base class HandHistory initialization
    initializeHandHistory(1);
    
    table->startNewHand();
    dealInitialCards();
    postBlinds();
    
    // Set initial player to act (UTG - 3 seats after dealer)
    int dealerPos = table->getDealerPosition();
    int playerCount = table->getPlayerCount();
    currentPlayerIndex = (dealerPos + 3) % playerCount;
}

void TexasHoldem::dealInitialCards() {
    table->dealHoleCards();
}

void TexasHoldem::runBettingRounds() {
    while (!isHandComplete()) {
        if (currentRound == BettingRound::PRE_FLOP) {
            std::cout << "\n=== PRE-FLOP ===" << std::endl;
            showGameState();
            completeBettingRound(HandHistoryRound::PRE_FLOP);
            nextPhase();
        } else if (currentRound == BettingRound::FLOP) {
            std::cout << "\n=== FLOP ===" << std::endl;
            table->dealFlop();
            showGameState();
            completeBettingRound(HandHistoryRound::FLOP);
            nextPhase();
        } else if (currentRound == BettingRound::TURN) {
            std::cout << "\n=== TURN ===" << std::endl;
            table->dealTurn();
            showGameState();
            completeBettingRound(HandHistoryRound::TURN);
            nextPhase();
        } else if (currentRound == BettingRound::RIVER) {
            std::cout << "\n=== RIVER ===" << std::endl;
            table->dealRiver();
            showGameState();
            completeBettingRound(HandHistoryRound::RIVER);
            currentRound = BettingRound::SHOWDOWN;
        }
    }
    
    // Conduct showdown if we reached it
    if (currentRound == BettingRound::SHOWDOWN) {
        conductShowdown();
    }
}

void TexasHoldem::conductShowdown() {
    table->showTable();
    table->showPotBreakdown();
    awardPotsStaged();
}

bool TexasHoldem::atShowdown() const {
    return currentRound == BettingRound::SHOWDOWN;
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
        smallBlindPlayer->addToInFor(config.smallBlind);
        table->setCurrentBet(config.smallBlind);
        
        // Record small blind using base class method
        recordPlayerAction(HandHistoryRound::PRE_HAND, smallBlindPlayer->getPlayerId(),
                          ActionType::POST_BLIND, config.smallBlind, 
                          "posts small blind $" + std::to_string(config.smallBlind));
        
        bigBlindPlayer->addToInFor(config.bigBlind);
        table->setCurrentBet(config.bigBlind);
        
        // Record big blind using base class method
        recordPlayerAction(HandHistoryRound::PRE_HAND, bigBlindPlayer->getPlayerId(),
                          ActionType::POST_BLIND, config.bigBlind,
                          "posts big blind $" + std::to_string(config.bigBlind));
        
        std::cout << smallBlindPlayer->getName() << " posts SB $" << config.smallBlind 
                  << ", " << bigBlindPlayer->getName() << " posts BB $" << config.bigBlind << std::endl;
    }
}


void TexasHoldem::nextPhase() {
    resetBettingRound();
    
    if (currentRound == BettingRound::PRE_FLOP) {
        currentRound = BettingRound::FLOP;
    } else if (currentRound == BettingRound::FLOP) {
        currentRound = BettingRound::TURN;
    } else if (currentRound == BettingRound::TURN) {
        currentRound = BettingRound::RIVER;
    } else if (currentRound == BettingRound::RIVER) {
        currentRound = BettingRound::SHOWDOWN;
    }
    
    // Set next player to act (first player after dealer for post-flop)
    if (currentRound != BettingRound::SHOWDOWN) {
        int dealerPos = table->getDealerPosition();
        int playerCount = table->getPlayerCount();
        currentPlayerIndex = (dealerPos + 1) % playerCount;
        
        // Find first active player
        int startIndex = currentPlayerIndex;
        while (true) {
            Player* player = table->getPlayer(currentPlayerIndex);
            if (player && !player->hasFolded() && !player->isAllIn()) {
                break;
            }
            currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
            if (currentPlayerIndex == startIndex) {
                currentPlayerIndex = -1; // No active players
                break;
            }
        }
    }
}

BettingRound TexasHoldem::getCurrentRound() const {
    return currentRound;
} 