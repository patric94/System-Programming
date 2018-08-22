#include "coord_header.h"

extern int *pool_pids, *pool_read_fds, *pool_write_fds;
extern int numOfPools;
extern char **statuses;
extern char *end;
int done_jobs = 0, currJobs = 0, term = 0;
JobList *myList;

int isNumber(int len, char * string){       //A fucntion to check if a given string is either an int or a float number.
    int k;
    char* s;
    s = string;
    for (k = 0 ;k < len; k++){
        if(!isdigit(*s) && *s != '.'){
            return 0;
        }
        s++;
    }
    return 1;
}

/*
    H synarthsh ayth vrethike apo to site :
	http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
*/
void  parse(char *line, char **argvu){
     while (*line != '\0') {       /* if not the end of line ....... */
          while (*line == ' ' || *line == '\t' || *line == '\n') {
			  *line++ = '\0';     /* replace white spaces with 0    */
		  }
          *argvu++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n') {
			  line++;             /* skip the argument until ...    */
		  }
     }
     *argvu = '\0';                 /* mark the end of argument list  */
}

void pool_handler(int sig) {
	int status, child_pid;

	while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		for (int i = 0; i < numOfPools; i++) {
			if (pool_pids[i] == child_pid) {
				pool_pids[i] = -1;
				break;
			}
		}
	}
}

void createPoolFifos(int numOfPools, char *dir, int **coord_read_fds, int **coord_write_fds, int **pool_read_fds, int **pool_write_fds){
	char fifoOUT[6], fifoIN[5];
	memset(fifoOUT, '\0', sizeof(fifoOUT));
	memset(fifoIN, '\0', sizeof(fifoIN));

	sprintf(fifoIN, "%dIN", numOfPools);// COORD : read from, POOL : write to.
	sprintf(fifoOUT, "%dOUT", numOfPools);// COORD : write to, POOL : read from.

	chdir(dir); //i make the fifo files for communication between pool and coord
	if ((mkfifo(fifoIN, 0666) == -1) && (errno != EEXIST)){
		perror("mkfifo jms_in error!");
		return;
	}
	if ((mkfifo(fifoOUT, 0666) == -1) && (errno != EEXIST)){
		perror("mkfifo jms_in error!");
		return;
	}

	//first i open the fifoIN pipe from both ends so both coord and pool can have access to.
	// COORD : read from, POOL : write to.
	if (((*coord_read_fds) = (int *)realloc(*coord_read_fds, numOfPools*sizeof(int))) == NULL) {
		free(coord_read_fds);
		perror("Error (re)allocating memory");
		return;
	}
	if (((*coord_read_fds)[numOfPools-1] = open(fifoIN, O_RDONLY | O_NONBLOCK)) < 0) {
		perror("Error ! Can't open pool_fifoIN!");
		return;
	}
	if (((*pool_write_fds) = (int *)realloc(*pool_write_fds, numOfPools*sizeof(int))) == NULL) {
		free(pool_read_fds);
		perror("Error (re)allocating memory");
		return;
	}
	if (((*pool_write_fds)[numOfPools-1] = open(fifoIN, O_WRONLY | O_NONBLOCK)) < 0) {
		perror("Error ! Can't open pool_fifoOUT!");
		return;
	}

	// printf("FD FOR %s ARE %d | %d\n", fifoIN, );

	//then i open the fifoOUT pipe from both ends for the above reason.
	// COORD : write to, POOL : read from.
	if (((*pool_read_fds) = (int *)realloc(*pool_read_fds, numOfPools*sizeof(int))) == NULL) {
		free(pool_read_fds);
		perror("Error (re)allocating memory");
		return;
	}
	if (((*pool_read_fds)[numOfPools-1] = open(fifoOUT, O_RDONLY | O_NONBLOCK)) < 0) {
		perror("Error ! Can't open pool_fifoIN!");
		return;
	}
	if (((*coord_write_fds) = (int *)realloc(*coord_write_fds, numOfPools*sizeof(int))) == NULL) {
		free(coord_write_fds);
		perror("Error (re)allocating memory");
		return;
	}
	if (((*coord_write_fds)[numOfPools-1] = open(fifoOUT, O_WRONLY | O_NONBLOCK)) < 0) {
		perror("Error ! Can't open pool_fifoIN!");
		return;
	}
	chdir("..");
}

void poolFunction(int currentJob, int poolNumber, int maxJobs, int read_fd, int write_fd, char *dir){
	char read_buf[MSG_BUF], write_buf[MSG_BUF]/*, temp[MSG_BUF], *token*/;
	struct sigaction shut;
	sigset_t shut_mask;
	memset(read_buf, '\0', MSG_BUF);
	memset(write_buf, '\0', MSG_BUF);

	sigemptyset(&shut_mask);
	sigaddset(&shut_mask, SIGCHLD);

	memset(&shut, 0, sizeof(shut));
	shut.sa_handler = shut_handler;
	shut.sa_mask = shut_mask;
	sigaction(SIGTERM, &shut, NULL);

	fd_set read_fds;
	int retval, bytes_in;

	FD_ZERO(&read_fds);
	FD_SET(read_fd, &read_fds);

	createJobList(&myList);

	int myPID = getpid();

	while (done_jobs < maxJobs && !term) {
		int job;
		retval = select(read_fd +1, &read_fds, NULL, NULL, NULL);
		if ( retval == -1 ) {
			if (errno == EINTR) { // if a signal interrupted the select call
				continue;
			}
			perror("select error");
			return;
		}
		else if ( retval ) {
			bytes_in = read(read_fd, read_buf, MSG_BUF);
			if (!bytes_in) { // if read returns zero
				break;
			}
			read_buf[bytes_in] = '\0';
			if ((job = fork()) == -1) {
				perror("job fork error");
				return;
			}
			else if (job == 0) {
				jobFunction(dir, read_buf, currentJob, write_buf, write_fd, myPID);
				freeJobList(myList);
			}
			currJobs++;
			addJobNodeToList(myList, job, 0);
			printf("\tPool %d executed job %d with jobID : %d : %s\n", poolNumber, currentJob, job, read_buf);
			currentJob++;
		}
	}

	freeJobList(myList);
}

void shut_handler(int sig){
	// printf("\treceived SIGTERM from coord\n");

	JobNode *aux;

	aux = myList->first;
	while (aux != NULL) {
		if (aux->status == 0) { // not finished
			printf("\tsending SIGTERM to %d\n", aux->pid);
			kill(aux->pid, SIGTERM);
		}
		else if (aux->status == 2) {
			kill(aux->pid, SIGCONT);
			printf("\tsending SIGTERM to %d\n", aux->pid);
			kill(aux->pid, SIGTERM);
		}
		aux = aux->next;
	}
	// printf("\tsend SIGTERM to all non-finished jobs\n");
	term = 1;
}

void job_handler(int sig){
	// printf("\tIn handler for SIGCHLD\n");
	int job_pid, my_pid;
	int status;
	char write_buf[MSG_BUF];
	memset(write_buf, '\0', MSG_BUF);

	// printf("\tPool %d received a signal that a job has died.\n", numOfPools);

	while ((job_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		printf("\t\tjob %d just finished\n", job_pid);
		updateStatus(myList, job_pid, 1);
		sprintf(write_buf, "finished %d %d", getpid(), job_pid);
		write(pool_write_fds[numOfPools-1], write_buf, MSG_BUF);
		currJobs--;
		done_jobs++;
	}
}

void jobFunction(char *dir, char *read_buf, int currentJob, char *write_buf, int write_fd, int parentPID){
	char *argvu[64];
	char date[9], ttime[7], job_out[10], job_err[10], folderName[50];
	struct stat st = {0};
	int jOut, jErr;
	memset(date, '\0', sizeof(date));
	memset(ttime, '\0', sizeof(ttime));
	memset(job_out, '\0', sizeof(job_out));
	memset(job_err, '\0', sizeof(job_err));

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(date, "%d%d%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	sprintf(ttime, "%d%d%d", tm.tm_hour, tm.tm_min, tm.tm_sec);
	sprintf(folderName, "jms_sdi1200025_%d_%d_%s_%s", currentJob, getpid(), date, ttime);
	sprintf(job_out, "stdout_%d", currentJob);
	sprintf(job_err, "stderr_%d", currentJob);

	chdir(dir);

	if ( stat(folderName, &st) == -1 ) {
		if (mkdir(folderName, 0700) != 0) {
			perror("job mkdir Error!");
			return;
		}
	}

	chdir(folderName);

	if ((jOut = open(job_out, O_CREAT | O_WRONLY)) < 0) {
		perror("Error ! Can't open job_out!");
		return;
	}
	if ((jErr = open(job_err, O_CREAT | O_WRONLY)) < 0) {
		perror("Error ! Can't open job_err!");
		return;
	}


	dup2(jOut, 1);
	dup2(jErr, 2);
	chdir("../..");

	sprintf(write_buf, "active %d %d %d %lu", parentPID, currentJob, getpid(), time(NULL));
	write(write_fd, write_buf, MSG_BUF);
	parse(read_buf, argvu);
	if ( execvp(*argvu, argvu) < 0 ) {
		perror("execvp error");
		return;
	}
}

void updateStatus(JobList *list, int pid, int status){
	JobNode *aux;

	aux = list->first;
	while (aux != NULL) {
		if (aux->pid == pid) {
			aux->status = status;
			break;
		}
		aux = aux->next;
	}
}

void findStatus(JobList *list, int id, int write_fd, char *write_buf) {
	JobNode *aux;
	int jobCount = 1;

	aux = list->first;
	while (aux != NULL) {
		if (jobCount == id) {
			if (aux->status) {
				sprintf(write_buf, "JobID: %d, PID: %d, Status : %s", jobCount, aux->pid, statuses[aux->status]);
			}
			else {
				sprintf(write_buf, "JobID: %d, PID: %d, Status : %s (running for %lu sec)", jobCount, aux->pid, statuses[aux->status], time(NULL)-aux->started);
			}
			write(write_fd, write_buf, MSG_BUF);
			break;
		}
		jobCount++;
		aux = aux->next;
	}
}

void findAllStatus(JobList *list, int write_fd, char *write_buf){
	JobNode *aux;
	int jobCount = 1;

	aux = list->first;
	while (aux != NULL) {
		sprintf(write_buf, "JobID: %d, PID: %d, Status : %s", jobCount, aux->pid, statuses[aux->status]);
		write(write_fd, write_buf, MSG_BUF);
		jobCount++;
		aux = aux->next;
	}
	write(write_fd, end, 15);
}

void findJobsByStatus(JobList *list, int status, int write_fd, char *write_buf) {
	JobNode *aux;
	int jobCount = 1, statusCount = 1;

	aux = list->first;
	sprintf(write_buf, "%s Jobs:", statuses[status]);
	write(write_fd, write_buf, MSG_BUF);
	while (aux != NULL) {
		if (aux->status == status) {
			sprintf(write_buf, "%d. JobID %d", statusCount, jobCount);
			write(write_fd, write_buf, MSG_BUF);
			statusCount++;
		}
		jobCount++;
		aux = aux->next;
	}
	write(write_fd, end, 15);
}

int sendSignalToJob(JobList *list, int id, int sig) {
	JobNode *aux;
	int jobCount = 1;

	aux = list->first;
	while (aux != NULL) {
		if (jobCount == id) {
			if (sig == SIGCONT && aux->status == 2) {
				aux->status = 0;
				aux->started = time(NULL);
			}
			else if (sig == SIGSTOP && aux->status == 0) {
				aux->status = 2;
			}
			else { //job is either active/suspended or finished.
				return -1;
			}
			kill(aux->pid, sig);
			return aux->pid;
		}
		jobCount++;
		aux = aux->next;
	}
}

void showPools(PoolList *list, int write_fd, char *write_buf) {
	PoolNode *aux;
	int poolCount = 1;

	aux = list->first;
	sprintf(write_buf, "Pools & NumOfJobs :");
	write(write_fd, write_buf, MSG_BUF);
	while (aux != NULL) {
		sprintf(write_buf, "%d. %d %d", poolCount, aux->pid, aux->currJobs);
		write(write_fd, write_buf, MSG_BUF);
		poolCount++;
		aux = aux->next;
	}
	write(write_fd, end, 15);
}

void createPoolList(PoolList **list) {
	(*list) = (PoolList *)malloc(sizeof(PoolList));
	(*list)->first = NULL;
	(*list)->last = NULL;
}

void addPoolNodeToList(PoolList *list, int mjobs, int pid) {

	PoolNode *node;

	node = (PoolNode*)malloc(sizeof(PoolNode));
	node->maxJobs = mjobs;
	node->pid = pid;
	node->currJobs = 0;
	node->next = NULL;

	if (list->first == NULL) { //if list is empty.
		list->first = node;
		list->last = node;
	}
	else { //if list contains at least one node.
		list->last->next = node;
		list->last = node;
	}
}

void updatePoolNode(PoolList *list, int pid, int mode){
	PoolNode *aux;
	aux = list->first;
	while (aux != NULL) {
		if (aux->pid == pid) {
			if (mode) {
				aux->currJobs++;
			}
			else {
				aux->currJobs--;
			}
			break;
		}
		aux = aux->next;
	}
}

void freePoolList(PoolList *list) {
	PoolNode *aux;

	while(list->first != NULL){
		aux = list->first->next;
		free(list->first);
		list->first = aux;
		if (aux != NULL) {
			aux = aux->next;
		}
	}
	free(list);
}

void createJobList(JobList **list) {
	(*list) = (JobList *)malloc(sizeof(JobList));
	(*list)->first = NULL;
	(*list)->last = NULL;
	(*list)->numOfJobs = 0;
	(*list)->activeJobs = 0;
}

void addJobNodeToList(JobList *list, int pid, int started){

	JobNode *node;

	node = (JobNode*)malloc(sizeof(JobNode));
	node->pid = pid;
	node->status = 0;
	node->started = started;
	node->next = NULL;

	if (list->first == NULL) { //if list is empty.
		list->first = node;
		list->last = node;
	}
	else { //if list contains at least one node.
		list->last->next = node;
		list->last = node;
	}
}

void freeJobList(JobList *list){
	JobNode *aux;

	while(list->first != NULL){
		aux = list->first->next;
		free(list->first);
		list->first = aux;
		if (aux != NULL) {
			aux = aux->next;
		}
	}
	free(list);
}
