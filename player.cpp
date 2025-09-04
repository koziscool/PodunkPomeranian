#include "player.h"
#include <iostream>

Player::Player(const std::string& playerName, int startingChips)
    : name(playerName), chips(startingChips), currentBet(0), folded(false), allIn(false) {}

const std::string& Player::getName() const {
    return name;
}

int Player::getChips() const {
    return chips;
}

const std::vector<Card>& Player::getHand() const {
    return hand;
}

int Player::getCurrentBet() const {
    return currentBet;
}

bool Player::hasFolded() const {
    return folded;
}

bool Player::isAllIn() const {
    return allIn;
}

int Player::getHandSize() const {
    return hand.size();
}

void Player::addCard(const Card& card) {
    hand.push_back(card);
}

void Player::clearHand() {
    hand.clear();
}

void Player::showHand() const {
    for (const auto& card : hand) {
        std::cout << card.toString() << " ";
    }
}

void Player::addChips(int amount) {
    chips += amount;
}

void Player::deductChips(int amount) {
    chips = std::max(0, chips - amount);
}

bool Player::canAfford(int amount) const {
    return chips >= amount;
}

PlayerAction Player::fold() {
    folded = true;
    return PlayerAction::FOLD;
}

PlayerAction Player::check() {
    return PlayerAction::CHECK;
}

PlayerAction Player::call(int callAmount) {
    int additionalAmount = callAmount - currentBet;
    if (additionalAmount >= chips) {
        return goAllIn();
    }
    
    deductChips(additionalAmount);
    currentBet = callAmount;
    return PlayerAction::CALL;
}

PlayerAction Player::raise(int raiseAmount) {
    int additionalAmount = raiseAmount - currentBet;
    if (additionalAmount >= chips) {
        return goAllIn();
    }
    
    deductChips(additionalAmount);
    currentBet = raiseAmount;
    return PlayerAction::RAISE;
}

PlayerAction Player::goAllIn() {
    currentBet += chips;
    chips = 0;
    allIn = true;
    return PlayerAction::ALL_IN;
}

void Player::resetBet() {
    currentBet = 0;
}

void Player::setBet(int amount) {
    currentBet = amount;
}

void Player::resetForNewHand() {
    hand.clear();
    currentBet = 0;
    folded = false;
    allIn = false;
}

void Player::showStatus(bool showCards) const {
    std::cout << std::setw(15) << name
              << " | Chips: " << std::setw(6) << chips
              << " | Bet: " << std::setw(4) << currentBet;

    if (showCards) {
        std::cout << " | Cards: ";
        if (hand.size() > 0) {
            std::string cardStr = "";
            for (const auto& card : hand) {
                cardStr += card.toString() + " ";
            }
            std::cout << std::setw(12) << std::left << cardStr << std::right;
        } else {
            std::cout << std::setw(12) << "(none)";
        }
    }
    if (folded) {
        std::cout << " | FOLDED";
    } else if (allIn) {
        std::cout << " | ALL-IN";
    }
    std::cout << std::endl;
} 