#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

#define MSG_BUF 256

typedef	struct ListParams{
	int sock;
	char *dir;
}ListParams;

typedef struct FetchParams{
	int sock;
	char *dir;
	char *requested;
	int delay;
}FetchParams;

typedef struct DelayNode {
	int id;
	int delay;
	struct DelayNode *next;
}DelayNode;

typedef struct DelayList {
	struct DelayNode *first;
	struct DelayNode *last;
}DelayList;

int isNumber(int, char*);
void initializeDelayList(DelayList **list);
void addDelayNodeToDelayList(DelayList *list, int id, int delay);
int retDelayFromDelayList(DelayList *list, int id);
void destroyDelayList(DelayList *list);
void *executingLIST(void *arg);
void sendFilesFromList(char *dir, int sock);
void show_dir_content(char *path, int sock);
void *executingFETCH(void *arg);
void fetchFile(char *dir, char *filePath, int sock, int delay);
