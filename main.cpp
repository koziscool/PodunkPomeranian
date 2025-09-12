#include "poker_game.h"
#include "table.h"
#include <iostream>
#include <memory>

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
    
    // Add players with IDs and personalities
    table.addPlayer("Alice", 1000, 0, PlayerPersonality::TIGHT_AGGRESSIVE);
    table.addPlayer("Bob", 1000, 1, PlayerPersonality::LOOSE_PASSIVE);
    table.addPlayer("Charlie", 1000, 2, PlayerPersonality::TIGHT_PASSIVE);
    table.addPlayer("Diana", 1000, 3, PlayerPersonality::LOOSE_AGGRESSIVE);
    table.addPlayer("Eve", 1000, 4, PlayerPersonality::LOOSE_AGGRESSIVE);
    table.addPlayer("Frank", 1000, 5, PlayerPersonality::TIGHT_AGGRESSIVE);
    
    // Set dealer position (Bob is dealer)
    table.advanceDealer(); // Move from 0 (Alice) to 1 (Bob)
    
    std::unique_ptr<PokerGame> game;
    
    // Create unified game based on choice using VariantInfo
    switch (choice) {
        case 1:
            std::cout << "\n=== TEXAS HOLD'EM (NL) - SINGLE HAND ===" << std::endl;
            std::cout << "Blinds: $10/$20" << std::endl;
            game = std::make_unique<PokerGame>(&table, PokerVariants::TEXAS_HOLDEM);
            break;
        case 2:
            std::cout << "\n=== 7-CARD STUD - SINGLE HAND ===" << std::endl;
            std::cout << "Ante: $5, Bring-in: $10, Small bet: $20, Large bet: $40" << std::endl;
            game = std::make_unique<PokerGame>(&table, PokerVariants::SEVEN_CARD_STUD);
            break;
        case 3:
            std::cout << "\n=== OMAHA HI-LO (8 OR BETTER) - SINGLE HAND ===" << std::endl;
            std::cout << "Blinds: $10/$20" << std::endl;
            game = std::make_unique<PokerGame>(&table, PokerVariants::OMAHA_HI_LO);
            break;
        default:
            std::cout << "Invalid choice. Defaulting to Texas Hold'em." << std::endl;
            game = std::make_unique<PokerGame>(&table, PokerVariants::TEXAS_HOLDEM);
            break;
    }
    
    // Track starting chip amounts for accurate gain/loss calculation
    std::vector<int> startingChips = {1000, 1000, 1000, 1000, 1000, 1000};
    
    // Multi-hand simulation
    const int NUM_HANDS = 5;
    VariantInfo variantInfo = game->getVariantInfo();
    
    for (int handNum = 1; handNum <= NUM_HANDS; handNum++) {
        std::cout << "\n=== HAND " << handNum << " ===" << std::endl;
        
        // Reset players for new hand
        for (int i = 0; i < table.getPlayerCount(); i++) {
            Player* player = table.getPlayer(i);
            if (player) {
                player->resetForNewHand();
            }
        }
        
        // Reset table state 
        table.setCurrentBet(0);
        table.getSidePotManager().clearPots();
        table.clearCommunityCards();
        
        // Advance button for board games only  
        if (variantInfo.gameStruct == GAMESTRUCTURE_BOARD && handNum > 1) {
            table.advanceDealer();
            std::cout << "Button advances to " << table.getPlayer(table.getDealerPosition())->getName() << std::endl;
        }
        
        // Start the hand
        game->startNewHand();
        
        // Show initial state (only for board games, Stud shows state with bring-in)
        if (variantInfo.gameStruct != GAMESTRUCTURE_STUD) {
            game->showGameState();
        }
        
        // Run the game
        game->runBettingRounds();
        
        // Award pot if hand is complete but showdown hasn't happened yet
        if (game->isHandComplete() && !game->atShowdown()) {
            // Everyone folded except one player - award pot without showdown
            game->awardPotsWithoutShowdown();
        } else if (!game->isHandComplete()) {
            // This shouldn't happen, but just in case
            game->conductShowdown();
        }
        
        std::cout << "\n=== END HAND " << handNum << " ===" << std::endl;
        
        // Remove broke players (those with 0 chips)
        std::vector<std::string> brokePlayerNames;
        for (int i = table.getPlayerCount() - 1; i >= 0; i--) {
            Player* player = table.getPlayer(i);
            if (player && player->getChips() == 0) {
                brokePlayerNames.push_back(player->getName());
                table.removePlayer(i);
            }
        }
        
        if (!brokePlayerNames.empty()) {
            std::cout << "\nPlayers eliminated (broke): ";
            for (size_t i = 0; i < brokePlayerNames.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << brokePlayerNames[i];
            }
            std::cout << std::endl;
        }
        
        // Check if we have enough players to continue
        if (table.getPlayerCount() < 2) {
            std::cout << "\nGame ended - not enough players remaining!" << std::endl;
            break;
        }
        
        // Show chip counts after each hand
        if (handNum < NUM_HANDS) {
            std::cout << "\n--- Chip Counts After Hand " << handNum << " ---" << std::endl;
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
        }
        
        // Reset and shuffle deck for next hand
        table.getDeck().reset();
        table.getDeck().shuffle();
    }
    
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
