OBJS 	= ContentFuncs.o ContentServer.o InitiatorFuncs.o MirrorInitiator.o ServerFuncs.o MirrorServer.o
SOURCE	= ContentFuncs.c ContentServer.c InitiatorFuncs.c MirrorInitiator.c ServerFuncs.c MirrorServer.c
HEADER  = ContentHeader.h InitiatorHeader.h ServerHeader.h
OUT = ContentServer MirrorInitiator MirrorServer
CC	= gcc
FLAGS   = -g -c -std=gnu99

all: $(OUT)

ContentServer : ContentServer.o ContentFuncs.o
	$(CC) ContentServer.o ContentFuncs.o -o ContentServer -pthread

MirrorInitiator : MirrorInitiator.o InitiatorFuncs.o
	$(CC) MirrorInitiator.o InitiatorFuncs.o -o MirrorInitiator -pthread

MirrorServer : MirrorServer.o ServerFuncs.o
	$(CC) MirrorServer.o ServerFuncs.o -o MirrorServer -pthread -lm

ContentFuncs.o: ContentFuncs.c
	$(CC) $(FLAGS) ContentFuncs.c

ContentServer.o: ContentServer.c
	$(CC) $(FLAGS) ContentServer.c

InitiatorFuncs.o : InitiatorFuncs.c
	$(CC) $(FLAGS) InitiatorFuncs.c

MirrorInitiator.o : MirrorInitiator.c
	$(CC) $(FLAGS) MirrorInitiator.c

ServerFuncs.o : ServerFuncs.c
	$(CC) $(FLAGS) ServerFuncs.c

MirrorServer.o : MirrorServer.c
	$(CC) $(FLAGS) MirrorServer.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)
	rm -r -f mplee
