#ifndef POKER_VARIANT_H
#define POKER_VARIANT_H

enum class PokerVariant {
    TEXAS_HOLDEM,
    SEVEN_CARD_STUD,
    OMAHA_HI_LO
};

struct VariantConfig {
    PokerVariant variant;
    int smallBlind;      // For Hold'em
    int bigBlind;        // For Hold'em
    int ante;           // For Stud
    int bringIn;        // For Stud
    int smallBet;       // For Stud
    int largeBet;       // For Stud
    
    // Constructor for Texas Hold'em
    VariantConfig(PokerVariant v, int sb, int bb) 
        : variant(v), smallBlind(sb), bigBlind(bb), ante(0), bringIn(0), smallBet(0), largeBet(0) {}
    
    // Constructor for 7-Card Stud
    VariantConfig(PokerVariant v, int a, int bi, int smb, int lb) 
        : variant(v), smallBlind(0), bigBlind(0), ante(a), bringIn(bi), smallBet(smb), largeBet(lb) {}
};

#endif 