CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra
TARGET = poker
TIE_TARGET = tie_demo
OBJS = main.o card.o deck.o player.o table.o game.o hand_evaluator.o side_pot.o
TIE_OBJS = tie_demo.o card.o deck.o player.o table.o game.o hand_evaluator.o side_pot.o

all: $(TARGET) $(TIE_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(TIE_TARGET): $(TIE_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TIE_TARGET) $(TIE_OBJS)

main.o: main.cpp game.h table.h
	$(CXX) $(CXXFLAGS) -c main.cpp

tie_demo.o: tie_demo.cpp game.h table.h
	$(CXX) $(CXXFLAGS) -c tie_demo.cpp

card.o: card.cpp card.h
	$(CXX) $(CXXFLAGS) -c card.cpp

deck.o: deck.cpp deck.h card.h
	$(CXX) $(CXXFLAGS) -c deck.cpp

player.o: player.cpp player.h card.h
	$(CXX) $(CXXFLAGS) -c player.cpp

table.o: table.cpp table.h player.h deck.h card.h side_pot.h
	$(CXX) $(CXXFLAGS) -c table.cpp

game.o: game.cpp game.h table.h player.h deck.h card.h hand_evaluator.h
	$(CXX) $(CXXFLAGS) -c game.cpp

hand_evaluator.o: hand_evaluator.cpp hand_evaluator.h card.h
	$(CXX) $(CXXFLAGS) -c hand_evaluator.cpp

side_pot.o: side_pot.cpp side_pot.h
	$(CXX) $(CXXFLAGS) -c side_pot.cpp

clean:
	rm -f $(OBJS) $(TIE_OBJS) $(TARGET) $(TIE_TARGET)

.PHONY: all clean
