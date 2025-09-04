#include "seven_card_stud.h"
#include "table.h"
#include <iostream>
#include <sstream>
#include <memory>

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
    std::cout << "=== 7-CARD STUD - SINGLE HAND ===" << std::endl;
    std::cout << "Ante: $5, Bring-in: $10, Small bet: $20, Large bet: $40" << std::endl;
    
    // Create table
    Table table;
    
    // Add players for stud
    table.addPlayer("Alice", 1000);
    table.addPlayer("Bob", 1000);
    table.addPlayer("Charlie", 1000);
    table.addPlayer("Diana", 1000);
    
    // Create 7-Card Stud game
    std::unique_ptr<SevenCardStud> game = std::make_unique<SevenCardStud>(&table, 5, 10, 20, 40);
    
    // Track starting chip amounts for accurate gain/loss calculation
    std::vector<int> startingChips = {1000, 1000, 1000, 1000};
    
    // Start buffering to capture hand output
    startBuffering();
    
    std::cout << "\n=== HAND 1 ===" << std::endl;
    
    // Start the hand
    game->startNewHand();
    
    // Show initial state
    game->showGameState();
    
    // Run the game
    game->runBettingRounds();
    
    // Conduct showdown if not already done
    if (!game->isHandComplete()) {
        game->conductShowdown();
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
