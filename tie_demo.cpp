#include "game.h"
#include "table.h"
#include <iostream>

int main() {
    std::cout << "=== TEXAS HOLD'EM TIE DEMO ===" << std::endl;
    
    // Create table and game
    Table table;
    Game game(&table);
    
    // Add players with equal stacks to make ties interesting
    table.addPlayer("Alice", 500);
    table.addPlayer("Bob", 500);
    table.addPlayer("Charlie", 500);
    table.addPlayer("Diana", 500);
    
    // Start the hand
    game.startNewHand();
    
    // Show initial state
    std::cout << "\n--- INITIAL STATE ---" << std::endl;
    game.showGameState();
    
    // Let's manually set up the community cards to force ties
    // We'll need to modify the deck or cards to create a specific scenario
    
    // For now, let's just play out the hand and see what happens
    std::cout << "\n--- SIMULATING BETTING ---" << std::endl;
    
    // Simple betting - everyone calls to see the showdown
    int currentPlayer = game.getCurrentPlayerIndex();
    
    // Player 1 action
    if (game.canPlayerAct(currentPlayer)) {
        game.playerCall(currentPlayer);
        currentPlayer = game.getCurrentPlayerIndex();
    }
    
    // Player 2 action  
    if (game.canPlayerAct(currentPlayer)) {
        game.playerCall(currentPlayer);
        currentPlayer = game.getCurrentPlayerIndex();
    }
    
    // Player 3 action
    if (game.canPlayerAct(currentPlayer)) {
        game.playerCall(currentPlayer);
        currentPlayer = game.getCurrentPlayerIndex();
    }
    
    // Player 4 action (big blind can check since everyone just called)
    if (game.canPlayerAct(currentPlayer)) {
        game.playerCheck(currentPlayer);
    }
    
    // Complete the hand properly through all betting rounds
    int phaseCounter = 0;
    while (!game.isHandComplete() && phaseCounter < 10) {
        std::cout << "Phase " << phaseCounter << ": Moving to next phase..." << std::endl;
        
        // Move to next phase (deals cards and starts betting if applicable)
        game.nextPhase();
        phaseCounter++;
        
        if (!game.isHandComplete()) {
            // Complete betting round - everyone checks
            int bettingCounter = 0;
            while (game.getCurrentPlayerIndex() != -1 && game.canPlayerAct(game.getCurrentPlayerIndex()) && bettingCounter < 20) {
                currentPlayer = game.getCurrentPlayerIndex();
                std::cout << "Player " << currentPlayer << " checks" << std::endl;
                game.playerCheck(currentPlayer);
                bettingCounter++;
            }
            
            if (bettingCounter >= 20) {
                std::cout << "Breaking out of betting loop!" << std::endl;
                break;
            }
        }
    }
    
    if (phaseCounter >= 10) {
        std::cout << "Breaking out of phase loop!" << std::endl;
    }
    
    // Show final chip counts
    std::cout << "\n=== FINAL CHIP COUNTS ===" << std::endl;
    for (int i = 0; i < table.getPlayerCount(); i++) {
        const Player* player = table.getPlayer(i);
        if (player) {
            std::cout << player->getName() << ": $" << player->getChips() << std::endl;
        }
    }
    
    return 0;
} 