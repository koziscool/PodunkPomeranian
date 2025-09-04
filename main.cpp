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
    std::cout << "=== TEXAS HOLD'EM POKER - 100 HAND SIMULATION ===" << std::endl;
    std::cout << "\n=== DISPLAYING HANDS WITH SIDE POTS OR CHOPPED POTS ===" << std::endl;
        
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
    int displayedHands = 0;
    
    for (int handNum = 1; handNum <= 100; handNum++) {
        // Always start buffering to capture output
        startBuffering();
        
        std::cout << "\n=== HAND " << handNum << " ===" << std::endl;
        
        // Start the hand
        game.startNewHand();
        
        // Show initial state
        game.showGameState();
        
        // Simple betting - everyone just calls to see showdown
        std::cout << "\n=== PRE-FLOP ===" << std::endl;
        int currentPlayer = game.getCurrentPlayerIndex();
        
        // Pre-flop: Manual betting to avoid loops
        // Manual betting to create the special scenario for hand 100 (like old hand 2)
        if (handNum == 100) {
            // Bob calls $20
            if (game.canPlayerAct(currentPlayer)) {
                game.playerCall(currentPlayer);
                currentPlayer = game.getCurrentPlayerIndex();
            }

            // Charlie calls $20  
            if (game.canPlayerAct(currentPlayer)) {
                game.playerCall(currentPlayer);
                currentPlayer = game.getCurrentPlayerIndex();
            }

            // Diana goes ALL-IN for $1000
            if (game.canPlayerAct(currentPlayer)) {
                game.playerAllIn(currentPlayer);
                currentPlayer = game.getCurrentPlayerIndex();
            }

            // Alice calls the all-in
            if (game.canPlayerAct(currentPlayer)) {
                game.playerCall(currentPlayer);
                currentPlayer = game.getCurrentPlayerIndex();
            }
            
            // Bob calls the all-in
            if (game.canPlayerAct(currentPlayer)) {
                game.playerCall(currentPlayer);
                currentPlayer = game.getCurrentPlayerIndex();
            }
            
            // Complete the rest with auto-complete
            game.autoCompleteCurrentBettingRound();
        }
        
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
        
        // Note: Real tie detection happens in the Game class during showdown
        // when hands are equally ranked and pots are split
        
        // Show results for this hand
        std::cout << "\nHand " << handNum << " chip counts:" << std::endl;
        for (int i = 0; i < table.getPlayerCount(); i++) {
            const Player* player = table.getPlayer(i);
            if (player) {
                int gained = player->getChips() - startingChips[i];
                std::cout << player->getName() << ": $" << player->getChips() 
                          << " (" << (gained >= 0 ? "+" : "") << gained << ")" << std::endl;
            }
        }
        
        std::cout << "\n=== END HAND " << handNum << " ===" << std::endl;
        
        // Check if this hand is interesting (side pots or chopped pots)
        bool isInteresting = game.isInterestingHand();
        
        // Display the hand if it's interesting, otherwise discard the buffer
        if (isInteresting) {
            endBuffering(true);  // Display the buffered output
            displayedHands++;
        } else {
            endBuffering(false);  // Discard the buffered output
        }
        
        // Keep chips cumulative - no reset between hands
    }
    
    std::cout << "\n=== SIMULATION COMPLETE ===" << std::endl;
    std::cout << "Displayed " << displayedHands << " interesting hands out of 100 total hands." << std::endl;
    
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
