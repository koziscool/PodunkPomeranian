#include "game.h"
#include "table.h"
#include <iostream>
#include <sstream>

// Global buffer for capturing output
std::stringstream outputBuffer;
std::streambuf* originalCout;

// Function to redirect cout to buffer
void startBuffering() {
    originalCout = std::cout.rdbuf();
    std::cout.rdbuf(outputBuffer.rdbuf());
}

// Function to restore cout and optionally flush buffer
void endBuffering(bool display = true) {
    std::cout.rdbuf(originalCout);
    if (display) {
        std::cout << outputBuffer.str();
    }
    outputBuffer.str("");  // Clear buffer
    outputBuffer.clear();  // Clear flags
}

int main() {
    std::cout << "=== TEXAS HOLD'EM POKER - SINGLE HAND ===" << std::endl;
        
    // Create table and game
    Table table;
    Game game(&table);
    
    // Add players with different stacks to test side pots
    table.addPlayer("Alice", 1000);
    table.addPlayer("Bob", 1000);
    table.addPlayer("Charlie", 150);  // Short stack
    table.addPlayer("Diana", 1000);
    
    // Track starting chip amounts for accurate gain/loss calculation
    std::vector<int> startingChips = {1000, 1000, 150, 1000};
    
    // Start buffering to capture hand output
    startBuffering();
    
    std::cout << "\n=== HAND 1 ===" << std::endl;
    
    // Start the hand
    game.startNewHand();
    
    // Show initial state
    game.showGameState();
    
    // Simple betting - everyone just calls to see showdown
    std::cout << "\n=== PRE-FLOP ===" << std::endl;
    
    // Complete pre-flop betting 
    while (!game.isHandComplete() && game.getCurrentRound() == BettingRound::PRE_FLOP) {
        if (game.getCurrentPlayerIndex() != -1) {
            game.autoCompleteCurrentBettingRound();
        }
        if (game.getCurrentRound() == BettingRound::PRE_FLOP) {
            game.nextPhase();
        }
    }
    
    // Complete the rest of the hand (only if not already complete)
    if (!game.isHandComplete()) {
        while (!game.isHandComplete()) {
            if (!game.isHandComplete() && game.getCurrentPlayerIndex() != -1) {
                game.autoCompleteCurrentBettingRound();
            }
            if (!game.isHandComplete()) {
                game.nextPhase();
            }
        }
    }
    
    // Show results for this hand
    std::cout << "\nHand 1 chip counts:" << std::endl;
    for (int i = 0; i < table.getPlayerCount(); i++) {
        const Player* player = table.getPlayer(i);
        if (player) {
            int gained = player->getChips() - startingChips[i];
            std::cout << player->getName() << ": $" << player->getChips() 
                      << " (" << (gained >= 0 ? "+" : "") << gained << ")" << std::endl;
        }
    }
    
    std::cout << "\n=== END HAND 1 ===" << std::endl;
    
    // End buffering and display the hand (option activated)
    endBuffering(true);  // Display the buffered hand output
    
    std::cout << "\n=== FINAL CHIP COUNTS ===" << std::endl;
    for (int i = 0; i < table.getPlayerCount(); i++) {
        const Player* player = table.getPlayer(i);
        if (player) {
            int gained = player->getChips() - startingChips[i];
            std::cout << player->getName() << ": $" << player->getChips() 
                      << " (" << (gained >= 0 ? "+" : "") << gained << ")" << std::endl;
        }
    }
    
    return 0;
}
