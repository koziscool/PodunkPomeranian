CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra
TARGET = poker
OBJS = main.o card.o deck.o player.o table.o poker_game.o texas_holdem.o seven_card_stud.o omaha_hi_lo.o hand_evaluator.o side_pot.o hand_history.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

main.o: main.cpp poker_game.h texas_holdem.h seven_card_stud.h omaha_hi_lo.h table.h
	$(CXX) $(CXXFLAGS) -c main.cpp


card.o: card.cpp card.h
	$(CXX) $(CXXFLAGS) -c card.cpp

deck.o: deck.cpp deck.h card.h
	$(CXX) $(CXXFLAGS) -c deck.cpp

player.o: player.cpp player.h card.h
	$(CXX) $(CXXFLAGS) -c player.cpp

table.o: table.cpp table.h player.h deck.h card.h side_pot.h
	$(CXX) $(CXXFLAGS) -c table.cpp

poker_game.o: poker_game.cpp poker_game.h table.h player.h hand_evaluator.h poker_variant.h
	$(CXX) $(CXXFLAGS) -c poker_game.cpp

texas_holdem.o: texas_holdem.cpp texas_holdem.h poker_game.h
	$(CXX) $(CXXFLAGS) -c texas_holdem.cpp

seven_card_stud.o: seven_card_stud.cpp seven_card_stud.h poker_game.h
	$(CXX) $(CXXFLAGS) -c seven_card_stud.cpp

omaha_hi_lo.o: omaha_hi_lo.cpp omaha_hi_lo.h poker_game.h poker_variant.h table.h player.h hand_evaluator.h
	$(CXX) $(CXXFLAGS) -c omaha_hi_lo.cpp

game.o: game.cpp game.h table.h player.h deck.h card.h hand_evaluator.h
	$(CXX) $(CXXFLAGS) -c game.cpp

hand_evaluator.o: hand_evaluator.cpp hand_evaluator.h card.h
	$(CXX) $(CXXFLAGS) -c hand_evaluator.cpp

side_pot.o: side_pot.cpp side_pot.h
	$(CXX) $(CXXFLAGS) -c side_pot.cpp

hand_history.o: hand_history.cpp hand_history.h card.h poker_variant.h
	$(CXX) $(CXXFLAGS) -c hand_history.cpp

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
