CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -O2
TARGET = avl_map_demo
OBJF = Objects
OBJ = $(OBJF)/avl_map.o $(OBJF)/main.o


all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJF)/avl_map.o: DataStructures/avl_map.c DataStructures/avl_map.h
	$(CC) $(CFLAGS) -c DataStructures/avl_map.c -o $(OBJF)/avl_map.o

$(OBJF)/main.o: main.c DataStructures/avl_map.h
	$(CC) $(CFLAGS) -c main.c -o $(OBJF)/main.o

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run