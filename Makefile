TARGET := main 
CC := g++ 

PREF_SRC := ./src/
PREF_OBJ := ./obj/

#create_src:
#	mkdir $(PREF_OBJ)

SRC := $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, $(PREF_OBJ)%.o, $(SRC)) 

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) 
#main: main.o 
#	g++ main.o -o main

$(PREF_OBJ)%.o: %.cpp
	$(CC) -c $< -o $@
#main.o: main.cpp 
#	$(CC) -c main.cpp -o main.o 

clean: 
	rm $(PREF_OBJ)*.o
#clean: 
#	rm $(TARGET) $(PREF_OBJ)*.o
#clean:
#	rm main *.o 
