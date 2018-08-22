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
#include <sys/select.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <dirent.h>
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

#define MSG_BUF 256

int isNumber(int, char*);
int make_conn_socket(int port, char *address);
