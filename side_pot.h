#ifndef SIDE_POT_H
#define SIDE_POT_H

#include <vector>
#include <set>

struct SidePot {
    int amount;
    int betLevel;
    std::set<int> eligiblePlayers;
    
    SidePot(int amt, int level) : amount(amt), betLevel(level) {}
};

class SidePotManager {
private:
    std::vector<SidePot> pots;
    
public:
    void clearPots();
    void createSidePotsFromBets(const std::vector<std::pair<int, int>>& playerBets);
    void addToPot(int amount, const std::set<int>& eligiblePlayers);
    void addToMainPot(int amount);
    
    int getTotalPotAmount() const;
    int getMainPotAmount() const;
    int getNumberOfPots() const;
    const std::vector<SidePot>& getPots() const;
    
    void showPotBreakdown() const;
};

#endif 