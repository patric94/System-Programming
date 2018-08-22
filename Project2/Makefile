OBJS 	= jms_coord.o jms_console.o coord_funcs.o console_funcs.o
SOURCE	= jms_coord.c jms_console.c coord_funcs.c console_funcs.c
HEADER  = coord_header.h console_header.h
OUT = jms_console jms_coord
CC	= gcc
FLAGS   = -g -c -std=c99

all: $(OUT)

jms_coord : jms_coord.o coord_funcs.o
	$(CC) jms_coord.o coord_funcs.o -o jms_coord

jms_console : jms_console.o console_funcs.o
	$(CC) jms_console.o console_funcs.o -o jms_console

jms_coord.o: jms_coord.c
	$(CC) $(FLAGS) -D_POSIX_C_SOURCE=199309L jms_coord.c

jms_console.o: jms_console.c
	$(CC) $(FLAGS) jms_console.c

coord_funcs.o : coord_funcs.c
	$(CC) $(FLAGS) -D_POSIX_C_SOURCE=199309L coord_funcs.c

console_funcs.o : console_funcs.c
	$(CC) $(FLAGS) console_funcs.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)
	rm -f fifo*
