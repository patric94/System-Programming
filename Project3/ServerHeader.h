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
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

char *strtok_r(char *str, const char *delim, char **saveptr);

#define MSG_BUF 256

typedef struct Buffer {
	int size;
	char **data;
	int count;
}Buffer;

typedef struct DirNode {
	char *cont;
	struct DirNode *next;
}DirNode;

typedef struct DirList {
	struct DirNode *first;
	struct DirNode *last;
}DirList;

typedef struct FileNode {
	int bytes;
	struct FileNode *next;
}FileNode;

typedef struct FileList {
	struct FileNode *first;
	struct FileNode *last;
}FileList;

void terminate(int sig);
int isNumber(int, char*);
int initializeBuffer(Buffer **buf, int size);
void destroyBuffer(Buffer *buf);
void initializeDirList(DirList **list);
void addDirNodeToDirList(DirList *list, char *cont);
void printDirList(DirList *list);
void destroyDirList(DirList *list);
void initializeFileList(FileList **list);
void addFileNodeToFileList(FileList *list, int bytes);
double getDispersion(FileList *list, double avg, int totalFiles);
void destroyFileList(FileList *list);
int make_conn_socket(int port, char *address);
int make_accept_socket(int *sock, int *readSock, int port);
int getInitiatorInfo(int readSock, char *dir);
void *MirrorManager(void *arg);
int placeInBuffer(DirList *list, int contID);
int validPath(char *dnContent, char *InitiatorContent);
void *Worker(void *arg);
void getFileFromContentServer(char *buf, char *dir);
int sigemptyset(sigset_t *set);

int sigaddset(sigset_t *set, int signum);

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
