
#ifndef VARIANTS_H
#define VARIANTS_H

#include <string>
#include <vector>

enum GameStructure
{
    GAMESTRUCTURE_BOARD,
    GAMESTRUCTURE_STUD,
};

enum NumHoleCards
{
    NUMHOLECARDS_NULL,
    NUMHOLECARDS_TWO,
    NUMHOLECARDS_FOUR
};

enum MaxBetsForLimit
{
    MAXLIMITBETS_4
};

enum Breakage
{
    POT_BREAK_TO_1
};

enum BestHandResolution
{
    BESTHANDRESOLUTION_ANYFIVE,
    BESTHANDRESOLUTION_TWOPLUSTHREE
};

enum PotResolution
{
    POTRESOLUTION_HIONLY,
    POTRESOLUTION_HILO_ACETO5,
    POTRESOLUTION_HILO_A5_MUSTQUALIFY
};

enum BettingStructure
{
    BETTINGSTRUCTURE_NO_LIMIT,
    BETTINGSTRUCTURE_LIMIT
};

// Unified betting round enum for all variants
enum UnifiedBettingRound
{
    UNIFIED_PRE_FLOP,        // Hold'em/Omaha pre-flop, Stud third street
    UNIFIED_FLOP,            // Hold'em/Omaha flop, Stud fourth street  
    UNIFIED_TURN,            // Hold'em/Omaha turn, Stud fifth street
    UNIFIED_RIVER,           // Hold'em/Omaha river, Stud sixth street
    UNIFIED_FINAL,           // Stud seventh street (board games done at river)
    UNIFIED_SHOWDOWN
};

struct VariantInfo
{
    std::string variantName;
    GameStructure gameStruct;
    NumHoleCards numHoleCards;
    std::vector<int> betSizes;
    BettingStructure bettingStruct;
    BestHandResolution handResolution;
    PotResolution potResolution;
};

// Predefined variant configurations
namespace PokerVariants {
    const VariantInfo TEXAS_HOLDEM = {
        "Texas Hold'em",
        GAMESTRUCTURE_BOARD,
        NUMHOLECARDS_TWO,
        {10, 20}, // smallBlind, bigBlind
        BETTINGSTRUCTURE_NO_LIMIT,
        BESTHANDRESOLUTION_ANYFIVE,
        POTRESOLUTION_HIONLY
    };
    
    const VariantInfo SEVEN_CARD_STUD = {
        "Seven Card Stud",
        GAMESTRUCTURE_STUD,
        NUMHOLECARDS_NULL,
        {5, 10, 20, 40}, // ante, bringIn, smallBet, bigBet
        BETTINGSTRUCTURE_LIMIT,
        BESTHANDRESOLUTION_ANYFIVE,
        POTRESOLUTION_HIONLY
    };
    
    const VariantInfo OMAHA_HI_LO = {
        "Omaha Hi-Lo",
        GAMESTRUCTURE_BOARD,
        NUMHOLECARDS_FOUR,
        {10, 20, 20, 40}, // smallBlind, bigBlind, smallBet, bigBet
        BETTINGSTRUCTURE_LIMIT,
        BESTHANDRESOLUTION_TWOPLUSTHREE,
        POTRESOLUTION_HILO_A5_MUSTQUALIFY
    };
}

#endif

