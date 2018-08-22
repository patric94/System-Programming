#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#define _GNU_SOURCE
#define MSG_BUF 256

typedef struct PoolNode {
	int pid;
	int maxJobs;
	int currJobs; // if this is 0 either pool is waiting for a job or has finished working.
	struct PoolNode *next;
}PoolNode;

typedef struct PoolList {
	struct PoolNode *first;
	struct PoolNode *last;
}PoolList;

typedef struct JobNode {
	int status; //status values : 0 is for Active, 1 is for Finished, 2 is for Suspended.
	int pid;
	int started;
	struct JobNode *next;
}JobNode;

typedef struct JobList {
	int numOfJobs;
	int activeJobs;
	struct JobNode *first;
	struct JobNode *last;
}JobList;

int isNumber(int, char*);
void parse(char *line, char **argvu);
void pool_handler(int sig);
void createPoolFifos(int numOfPools, char *dir, int **coord_read_fds, int **coord_write_fds, int **pool_read_fds, int **pool_write_fds);
void poolFunction(int currentJob, int poolNumber, int maxJobs, int read_fd, int write_fd, char *dir);
void shut_handler(int sig);
void job_handler(int sig);
void jobFunction(char *dir, char *read_buf, int currentJob, char *write_buf, int write_fd, int parentPID);
void updateStatus(JobList *list, int pid, int status);
void findStatus(JobList *list, int id, int write_fd, char *write_buf);
void findAllStatus(JobList *list, int write_fd, char *write_buf);
void findJobsByStatus(JobList *list, int status, int write_fd, char *write_buf);
int sendSignalToJob(JobList *list, int id, int sig);
void showPools(PoolList *list, int write_fd, char *write_buf);
void createPoolList(PoolList **list);
void addPoolNodeToList(PoolList *list, int mjobs, int pid);
void updatePoolNode(PoolList *list, int pid, int mode);
void freePoolList(PoolList *list);
void createJobList(JobList **list);
void addJobNodeToList(JobList *list, int pid, int started);
void freeJobList(JobList *list);
int sigemptyset(sigset_t *set);

int sigaddset(sigset_t *set, int signum);

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

int kill(pid_t pid, int sig);
