#include "coord_header.h"

int *pool_pids = NULL, *pool_read_fds = NULL, *pool_write_fds = NULL;
int numOfPools = 0;
char **statuses;
char *end;

int main(int argc, char *argv[]) {
	struct stat st = {0};
	int flagW = 0, flagR = 0, flagL = 0, flagN = 0; // Flags in order to check which operation characters have been put as input.
	char read_buf[MSG_BUF], write_buf[MSG_BUF], *dir; // String that keeps the current working directory and a buffer for the queries.
	memset(read_buf, '\0', MSG_BUF);
	memset(write_buf, '\0', MSG_BUF);
	char *jms_in, *jms_out;
	int write_fd, read_fd, *coord_read_fds = NULL, *coord_write_fds = NULL, max_fd;
	int fifoOpened = 0, maxJobs;
	JobList *list;
	PoolList *plist;

	if ( argc != 9 ) {
		fprintf(stderr, "Error ! Wrong input of arguments\n");
        return 1;
	}

	//A 'for' loop in order to check the in-line arguments.
	for (int i = 1; i < argc; i+= 2) {
		if ( strcmp(argv[i], "-w") == 0 ) {
            flagW = 1;
			jms_out = argv[i+1];
			if ((mkfifo(jms_out, 0666) == -1) && (errno != EEXIST)){
            	perror("mkfifo jms_out error!");
            	return 1;
        	}
        }
		else if ( strcmp(argv[i], "-r") == 0 ) {
			flagR = 1;
			jms_in = argv[i+1];
			if ((mkfifo(jms_in, 0666) == -1) && (errno != EEXIST)){
            	perror("mkfifo jms_in error!");
            	return 1;
        	}
			if ((read_fd = open(jms_in, O_RDONLY | O_NONBLOCK)) < 0) {
				perror("Error ! Can't open jms_in!");
				return 1;
			}
		}
		else if ( strcmp(argv[i], "-l") == 0 ) { //In this argument I expect a name of the folder to be created
			flagL = 1;							 //so i dont accept the absolute path.
			if (( dir = (char *) malloc(256*sizeof(char))) == NULL) {
                perror("memory for current directory!");
                return 1;
            }//Allocating enough space for both currnet path and the temp directory and the names of fifo files.
            getcwd(dir, 256);
            strcat(dir, "/");
            strcat(dir, argv[i+1]);
            if ( stat(dir, &st) == -1 ) {
                if (mkdir(dir, 0700) != 0) {
                    perror("mkdir Error!");
                }
            }
            strcat(dir, "/");
		}
		else if ( strcmp(argv[i], "-n") == 0 ) {
			flagN = 1;
			if ( !isNumber(strlen(argv[i+1]), argv[i+1]) ) {
                fprintf(stderr, "Error ! jobs_pool must be an integer value !\n");
                return 1;
            }
			maxJobs = atoi(argv[i+1]);
		}
		else {
			fprintf(stderr, "Error ! Unacceptable in-line argument!\n");
            return 1;
		}
	}

	if ( !flagW || !flagR || !flagL || !flagN ) {
		fprintf(stderr, "Error ! Wrong input of arguments ! Missing a mandatory one !\n");
        return 1;
	}

	if (( statuses = (char **) malloc(3*sizeof(char*))) == NULL) {
		perror("memory for statuses!");
		return 1;
	}
	for (int i = 0; i < 3; i++) {
		if ((statuses[i] = (char *)malloc(10*sizeof(char))) == NULL) {
			perror("memory for statuses!");
			return 1;
		}
		memset(statuses[i], '\0', 10);
	}
	sprintf(statuses[0], "Active");
	sprintf(statuses[1], "Finished");
	sprintf(statuses[2], "Suspended");
	if ((end = (char *)malloc(15*sizeof(char))) == NULL) {
		perror("memory for statuses!");
		return 1;
	}
	sprintf(end, "end_of_command");

	struct sigaction sa;
	sigset_t block_mask;

	sigemptyset(&block_mask);
	sigaddset(&block_mask, SIGCHLD);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = pool_handler;
	sa.sa_mask = block_mask;

	sigaction(SIGCHLD, &sa, NULL);

	fd_set read_fds;
	struct timeval tv;
	int retval, bytes_in;

	FD_ZERO(&read_fds);
	FD_SET(read_fd, &read_fds);
	max_fd = read_fd;

	createJobList(&list); //A list that contains all jobs that are going to be executed.
	createPoolList(&plist); //A list that contains all pool that are going to be created.

	while (1) {
		retval = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if ( retval == -1 ) {
			if (errno == EINTR) {
				continue;
			}
			perror("select error");
			return 1;
		}
		else if ( retval ) {
			if (FD_ISSET(read_fd, &read_fds)) { //reading from the jms_console pipe.
				char temp[MSG_BUF], *token;
				bytes_in = read(read_fd, read_buf, MSG_BUF);
				if (!bytes_in) { // if read returns zero
					break;
				}
				read_buf[bytes_in] = '\0';
				fifoOpened++;
				// because the FIFO files, according to man pages, in order to be able to open from the write end
				// have to be open from the read end, so i wait for the first input i will get from the jms_console.
				if (fifoOpened == 1) {
					if ((write_fd = open(jms_out, O_WRONLY | O_NONBLOCK)) < 0) {
						perror("Error ! Can't open jms_out!");
						return 1;
					}
				}
				strcpy(temp, read_buf);
				token = strtok(temp," ");
				fprintf(stdout, "%s\n", read_buf);
				if (strcmp(token, "submit") == 0) {
					char *input;
					input = temp;
					int place = strlen(token);
					input += place+1;
					list->numOfJobs++; //One new job is going to be executed from a pool
					if (maxJobs*numOfPools < list->numOfJobs) { // create a pool to handle the job.
						int pl;
						numOfPools++;
						createPoolFifos(numOfPools, dir, &coord_read_fds, &coord_write_fds, &pool_read_fds, &pool_write_fds);
						write(coord_write_fds[numOfPools-1], input, strlen(input));
						if ((pl = fork()) == -1) {
							perror("pool fork error!");
							return 1;
						}
						else if (pl == 0) {
							struct sigaction sa1;
							sigset_t block_mask1;

							sigemptyset(&block_mask1);
							sigaddset(&block_mask1, SIGCHLD);

							memset(&sa1, 0, sizeof(sa1));

							sa1.sa_handler = job_handler;
							sa1.sa_mask = block_mask1;

							sigaction(SIGCHLD, &sa1, NULL);
							// createPoolNode(maxJobs, getpid(), numOfPools, &pn);
							poolFunction(list->numOfJobs, numOfPools, maxJobs, pool_read_fds[numOfPools-1], pool_write_fds[numOfPools-1], dir);
							printf("\tPool %d with pid %d finished work.\n", numOfPools, getpid());
							for (int i = 0; i < 3; i++) {
								free(statuses[i]);
							}
							free(statuses);
							free(end);
							for (int i = 0; i < numOfPools; i++) {
								close(coord_read_fds[i]);
								close(coord_write_fds[i]);
								close(pool_read_fds[i]);
								close(pool_write_fds[i]);
							}
							free(pool_pids);
							free(coord_read_fds);
							free(coord_write_fds);
							free(pool_read_fds);
							free(pool_write_fds);
							freeJobList(list);
							freePoolList(plist);
							free(dir);
							close(write_fd);
							close(read_fd);
							exit(1);
						}
						addPoolNodeToList(plist, maxJobs, pl);
						if ((pool_pids = (int *)realloc(pool_pids, numOfPools*sizeof(int))) == NULL) {
							free(pool_pids);
							perror("Error (re)allocating memory");
							return 1;
						}
						pool_pids[numOfPools-1] = pl;
					}
					else { // assign the job to the last pool
						write(coord_write_fds[numOfPools-1], input, strlen(input));
					}
				}
				else if (strcmp(token, "status") == 0) {
					if ((token = strtok(NULL," ")) == NULL) {
						sprintf(write_buf, "Error ! Missing argument!");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					int jobID;
					if (!isNumber(strlen(token), token)) {
						sprintf(write_buf, "Error ! JobID must be a positive integer !");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					jobID = atoi(token);
					if (jobID > list->numOfJobs) {
						sprintf(write_buf, "Error ! JobID %d doesn't exist !", jobID);
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					findStatus(list, jobID, write_fd, write_buf);
				}
				else if (strcmp(token, "status-all") == 0) {
					printf("Printing Job Statuses to Console\n");
					findAllStatus(list, write_fd, write_buf);
				}
				else if (strcmp(token, "show-active") == 0) {
					printf("Printing Active Jobs to Console\n");
					findJobsByStatus(list, 0, write_fd, write_buf);
				}
				else if (strcmp(token, "show-pools") == 0) {
					printf("Printing Pools to Console\n");
					showPools(plist, write_fd, write_buf);
				}
				else if (strcmp(token, "show-finished") == 0) {
					printf("Printing Finished Jobs to Console\n");
					findJobsByStatus(list, 1, write_fd, write_buf);
				}
				else if (strcmp(token, "suspend") == 0) {
					if ((token = strtok(NULL," ")) == NULL) {
						sprintf(write_buf, "Error ! Missing argument!");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					int jobID;
					if (!isNumber(strlen(token), token)) {
						sprintf(write_buf, "Error ! JobID must be a positive integer !");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					jobID = atoi(token);
					if (jobID > list->numOfJobs) {
						sprintf(write_buf, "Error ! JobID %d doesn't exist !", jobID);
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					if (sendSignalToJob(list, jobID, SIGSTOP) != -1) {
						sprintf(write_buf, "Sent suspend signal to JobID %d", jobID);
					}
					else {
						sprintf(write_buf, "JobID %d cannot be suspended!", jobID);
					}
					write(write_fd, write_buf, MSG_BUF);
				}
				else if (strcmp(token, "resume") == 0) {
					if ((token = strtok(NULL," ")) == NULL) {
						sprintf(write_buf, "Error ! Missing argument!");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					int jobID;
					if (!isNumber(strlen(token), token)) {
						sprintf(write_buf, "Error ! JobID must be a positive integer !");
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					jobID = atoi(token);
					if (jobID > list->numOfJobs) {
						sprintf(write_buf, "Error ! JobID %d doesn't exist !", jobID);
						printf("%s\n", write_buf);
						write(write_fd, write_buf, MSG_BUF);
						continue;
					}
					if (sendSignalToJob(list, jobID, SIGCONT) != -1) {
						sprintf(write_buf, "Sent resume signal to JobID %d", jobID);
					}
					else {
						sprintf(write_buf, "JobID %d cannot be resumed!", jobID);
					}
					write(write_fd, write_buf, MSG_BUF);
				}
				else if (strcmp(token, "shutdown") == 0) {
					for (int i = 0; i < numOfPools; i++) {
						if (pool_pids[i] != -1) {
							int status;
							printf("Sending SIGTERM to %d\n", pool_pids[i]);
							kill(pool_pids[i], SIGTERM);
							waitpid(pool_pids[i], &status, 0);
						}
					}
					sprintf(write_buf, "Served %d jobs, %d were still in progress", list->numOfJobs, list->activeJobs);
					write(write_fd, write_buf, MSG_BUF);
					break;
				}
				else {
					fprintf(stderr, "Error ! Unacceptable command for the programme !\n");
					sprintf(write_buf, "Error ! Unacceptable command for the programme !");
					write(write_fd, write_buf, MSG_BUF);
					write(write_fd, end, 15);
					continue;
				}
			}
			else {
				//in this 'for' coord reads from the pipes the programme has between its pools and
				//updates the information it has for the running pool/jobs
				for (int i = 0; i < numOfPools; i++) {
					if (FD_ISSET(coord_read_fds[i], &read_fds)) {
						char *token;
						while ((bytes_in = read(coord_read_fds[i], read_buf, MSG_BUF)) > 0) {
							token = strtok(read_buf," ");
							if (strcmp(token, "active") == 0) {
								int id, jpid, ppid, started;
								token = strtok(NULL, " ");
								ppid = atoi(token);
								token = strtok(NULL, " ");
								id = atoi(token);
								token = strtok(NULL, " ");
								jpid = atoi(token);
								token = strtok(NULL, " ");
								started = atoi(token);

								list->activeJobs++;
								addJobNodeToList(list, jpid, started);
								updatePoolNode(plist, ppid, 1);
								sprintf(write_buf, "JobID: %d, PID: %d", id, jpid);
								write(write_fd, write_buf, MSG_BUF);
							}
							else if (strcmp(token, "finished") == 0) {
								int pid, ppid;
								token = strtok(NULL, " ");
								ppid = atoi(token);
								token = strtok(NULL, " ");
								pid = atoi(token);
								list->activeJobs--;
								updateStatus(list, pid, 1);
								updatePoolNode(plist, ppid, 0);
							}
						}
					}
				}
			}
		}
		FD_ZERO(&read_fds);
		FD_SET(read_fd, &read_fds);
		max_fd = read_fd;
		for (int i = 0; i < numOfPools; i++) {
			FD_SET(coord_read_fds[i], &read_fds);
			if (coord_read_fds[i] > max_fd) {
				max_fd = coord_read_fds[i];
			}
		}
	}
	for (int i = 0; i < 3; i++) {
		free(statuses[i]);
	}
	free(statuses);
	free(end);
	for (int i = 0; i < numOfPools; i++) {
		close(coord_read_fds[i]);
		close(coord_write_fds[i]);
		close(pool_read_fds[i]);
		close(pool_write_fds[i]);
	}
	free(pool_pids);
	free(coord_read_fds);
	free(coord_write_fds);
	free(pool_read_fds);
	free(pool_write_fds);
	freeJobList(list);
	freePoolList(plist);
	free(dir);
	close(write_fd);
	close(read_fd);
	return 0;
}
