#include "poker_game.h"
#include "texas_holdem.h"
#include "seven_card_stud.h"
#include "omaha_hi_lo.h"
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
    std::cout << "=== POKER VARIANTS ===" << std::endl;
    std::cout << "Select a poker variant:" << std::endl;
    std::cout << "1. Texas Hold'em (NL)" << std::endl;
    std::cout << "2. 7-Card Stud" << std::endl;
    std::cout << "3. Omaha Hi-Lo (8 or Better)" << std::endl;
    std::cout << "Enter choice (1-3): ";
    
    int choice;
    std::cin >> choice;
    
    // Create table
    Table table;
    
    // Add players
    table.addPlayer("Alice", 1000);
    table.addPlayer("Bob", 1000);
    table.addPlayer("Charlie", 1000);
    table.addPlayer("Diana", 1000);
    
    // Set dealer position (Bob is dealer)
    table.advanceDealer(); // Move from 0 (Alice) to 1 (Bob)
    
    std::unique_ptr<PokerGame> game;
    
    // Create appropriate game based on choice
    switch (choice) {
        case 1:
            std::cout << "\n=== TEXAS HOLD'EM (NL) - SINGLE HAND ===" << std::endl;
            std::cout << "Blinds: $10/$20" << std::endl;
            game = std::make_unique<TexasHoldem>(&table);
            break;
        case 2:
            std::cout << "\n=== 7-CARD STUD - SINGLE HAND ===" << std::endl;
            std::cout << "Ante: $5, Bring-in: $10, Small bet: $20, Large bet: $40" << std::endl;
            game = std::make_unique<SevenCardStud>(&table, 5, 10, 20, 40);
            break;
        case 3:
            std::cout << "\n=== OMAHA HI-LO (8 OR BETTER) - SINGLE HAND ===" << std::endl;
            std::cout << "Blinds: $10/$20" << std::endl;
            game = std::make_unique<OmahaHiLo>(&table);
            break;
        default:
            std::cout << "Invalid choice. Defaulting to Texas Hold'em." << std::endl;
            game = std::make_unique<TexasHoldem>(&table);
            break;
    }
    
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
    
    std::cout << "\n=== END HAND 1 ===" << std::endl;
    
    // End buffering and display the hand
    endBuffering(true);
    
    // Display final chip counts
    std::cout << "\n=== FINAL CHIP COUNTS ===" << std::endl;
    for (int i = 0; i < table.getPlayerCount(); i++) {
        Player* player = table.getPlayer(i);
        if (player) {
            int gain = player->getChips() - startingChips[i];
            std::cout << player->getName() << ": $" << player->getChips();
            if (gain > 0) {
                std::cout << " (+$" << gain << ")";
            } else if (gain < 0) {
                std::cout << " (-$" << (-gain) << ")";
            }
            std::cout << std::endl;
        }
    }
    
    return 0;
}
