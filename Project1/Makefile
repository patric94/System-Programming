OBJS 	= werhauz.o functions.o myStructures.o
SOURCE	= werhauz.c functions.c myStructures.c
HEADER  = fucntion.h myStructures.h
OUT = werhauz
CC	= gcc
FLAGS   = -g -c -std=c99

all: $(OUT)

werhauz : werhauz.o functions.o myStructures.o
	$(CC) werhauz.o functions.o myStructures.o -o werhauz -lm

werhauz.o: werhauz.c
	$(CC) $(FLAGS) werhauz.c

functions.o : functions.c
	$(CC) $(FLAGS) functions.c -lm

myStructures.o: myStructures.c
	$(CC) $(FLAGS) myStructures.c -lm
# clean house
clean:
	rm -f $(OBJS) $(OUT)
