#include "ServerHeader.h"
#define SIZE 10 // fixed size of the buffer.

int ContentServersRequests = 0; // counts the arguments amount of flag -s in the MirrorInitiator programme.
int started = 0, working = 1; // variables to declare the stated of the server.
char **ServerAdd, **dirOrfiles, **dirNames;  // arrays to store the information provided
int *delays, *ServerPort;					//  from the MirrorInitiator.
int endOfExecution = 0;  // variable that signals the workers to stop execution.
int bytesTransferred, filesTransferred, numWorkersDone, managersDone;
FileList *flist;
Buffer *buffer; // the fixed buffer used for manager/worker communication.
pthread_mutex_t buffer_mtx, devices_mtx, bytes_mtx, managers_mtx;
pthread_cond_t empty_cv, full_cv, allDone_cv;

int main(int argc, char *argv[]) {
	struct stat st = {0};
	char *dir;
	int workers, portNum, err;
	int flagW, flagP, flagM; // Flags in order to check which operation characters have been put as input.
	flagM = flagP = flagW = 0;
	char buf[MSG_BUF];
	memset(buf, '\0', MSG_BUF);
	pthread_t *managers_tids, *workers_tids; // arrays that hold the threads.

	if ( argc != 7 ) {
        fprintf(stderr, "Error ! Wrong input of arguments\n");
        return 1;
    }
	for (int i = 1; i < argc; i+=2) {
		if (strcmp(argv[i], "-p") == 0) {
			if ( !isNumber(strlen(argv[i+1]), argv[i+1]) ) {
                fprintf(stderr, "Error ! Port Number must be an integer value !\n");
                return 1;
            }
			flagP = 1;
			portNum = atoi(argv[i+1]);
		}										//In this argument I expect a name of the folder to be created
		else if (strcmp(argv[i], "-m") == 0) {  //so i dont accept the absolute path.
			if (( dir = (char *) malloc(256*sizeof(char))) == NULL) {
                fprintf(stderr, "Error ! Not enough memory for current directory!\n");
                return 0;
            }//Allocating enough space for both currnet path and the temp directory and the names of fifo files.
            getcwd(dir, 256);
            strcat(dir, "/");
            strcat(dir, argv[i+1]);
            if ( stat(dir, &st) == -1 ) {
                if (mkdir(dir, 0700) != 0) {
                    perror("mkdir Error!");
					return 1;
                }
            }
            strcat(dir, "/");
            flagM = 1;
		}
		else if (strcmp(argv[i], "-w") == 0) {
			if ( !isNumber(strlen(argv[i+1]), argv[i+1]) ) {
                fprintf(stderr, "Error ! Workers(Threads) count must be an integer value !\n");
                return 1;
            }
			flagW = 1;
            workers = atoi(argv[i+1]);
		}
		else {
            fprintf(stderr, "Error ! Unacceptable in-line argument!\n");
            return 1;
        }
	}

	if ( !flagP || !flagM || !flagW ) {
		fprintf(stderr, "Error ! Wrong input of arguments ! Missing a mandatory one !\n");
		return 1;
	}
	/*I use the SIGTERM (kill -15 pid) in order to terminate the server */
	struct sigaction a;
	a.sa_handler = terminate;
	a.sa_flags = 0;
	sigemptyset( &a.sa_mask );
	sigaction( SIGTERM, &a, NULL );

	/* Initialize mutex and condition variables*/
	pthread_mutex_init(&buffer_mtx, NULL);
	pthread_mutex_init(&devices_mtx, NULL);
	pthread_mutex_init(&bytes_mtx, NULL);
	pthread_mutex_init(&managers_mtx, NULL);
	pthread_cond_init(&empty_cv, NULL);
	pthread_cond_init(&full_cv, NULL);
	pthread_cond_init(&allDone_cv, NULL);

	int sock, readSock;

	struct sockaddr_in server, client;
	socklen_t clientlen;
	struct sockaddr *serverptr=(struct sockaddr *)&server;
	struct sockaddr *clientptr=(struct sockaddr *)&client;
	struct hostent *rem;

	/* Create socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}
	server.sin_family = AF_INET;       /* Internet domain */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(portNum);      /* The given port */
	/* Bind socket to address */
	if (bind(sock, serverptr, sizeof(server)) < 0) {
		perror("bind");
		return 1;
	}
	/* Listen for connections */
	if (listen(sock, 5) < 0) {
		perror("listen");
		return 1;
	}
	printf("Listening for connections to port %d\n", portNum);
	if (initializeBuffer(&buffer, SIZE) < 0) {
		return -1;
	}
	printf("Creating %d Worker threads\n", workers);
	if ((workers_tids = (pthread_t *)malloc(workers * sizeof(pthread_t))) == NULL) {
		perror("workers_tids malloc");
		return 1;
	}
	for (int i = 0; i < workers; i++) {
		//Creating Workers here.
		if (err = pthread_create(workers_tids+i, NULL, Worker, (void *) dir)) {
			perror2("pthread_create", err);
			return 1;
		}
	}
	while (working) {
		clientlen = sizeof(client);
		/* accept connection */
		printf("Server %d at your service ! :)\n", getpid());
		if ((readSock = accept(sock, clientptr, &clientlen)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("accept");
			return 1;
		}

		ServerAdd = NULL; dirOrfiles = NULL; dirNames = NULL;
		delays = NULL; ServerPort = NULL;
		bytesTransferred = 0; filesTransferred = 0; numWorkersDone = 0; managersDone = 0;
		flist = NULL;

		if (getInitiatorInfo(readSock, dir) < 0) {
			return 1;
		}

		initializeFileList(&flist);

		started = 1;
		printf("Creating %d MirrorManager threads\n", ContentServersRequests);
		if ((managers_tids = (pthread_t *)malloc(ContentServersRequests * sizeof(pthread_t))) == NULL) {
			perror("managers_tids malloc");
			return 1;
		}
		for (int i = 0; i < ContentServersRequests; i++) {
			printf("Created MirrorManager with id: %d\n", i);
			int *arg = malloc(sizeof(int));
			*arg = i;
			//Creating MirrorManagers here.
			if (err = pthread_create(managers_tids+i, NULL, MirrorManager, (void *) arg)) {
				perror2("pthread_create", err);
				return 1;
			}
		}

		pthread_mutex_lock(&devices_mtx);

		while (numWorkersDone != workers) {
			pthread_cond_wait(&allDone_cv, &devices_mtx);
		}
		pthread_mutex_lock(&bytes_mtx);
		double avg = bytesTransferred/filesTransferred;
		double dispersion = getDispersion(flist, avg, filesTransferred);
		sprintf(buf, "Transfered %d Files of %d Bytes from %d Request(s). AVGFileSize %.2f Dispersion %.2f\n", filesTransferred, bytesTransferred, ContentServersRequests, avg, dispersion);
		write(readSock, buf, MSG_BUF);
		pthread_mutex_unlock(&bytes_mtx);
		pthread_mutex_unlock(&devices_mtx);

		/*Re-Initializing for a new connection!*/

		destroyFileList(flist);
		free(managers_tids);
		started = 0;
		for (int i = 0; i < ContentServersRequests; i++) {
			free(ServerAdd[i]);
			free(dirOrfiles[i]);
			free(dirNames[i]);
		}
		ContentServersRequests = 0;
		free(ServerAdd);
		free(dirOrfiles);
		free(dirNames);
		free(ServerPort);
		free(delays);
	}


	// The server has been signaled to stop its execution.
	// pthread_mutex_lock(&buffer_mtx);
	endOfExecution = 1;
	// pthread_mutex_unlock(&buffer_mtx);
	pthread_cond_broadcast(&empty_cv);
	printf("Waiting for workers to terminate\n");
	for (int i = 0; i < workers; i++) {
		if ((err = pthread_join(*(workers_tids+i), NULL))) {
			perror2("pthread_join", err);
		    exit(1);
		}
		// pthread_cond_signal(&empty_cv);
	}


	// pthread_mutex_unlock(&buffer_mtx);
	if (err = pthread_mutex_destroy(&buffer_mtx)) {
        perror2("pthread_mutex_destroy1", err);
		// return 1;
	}
    if (err = pthread_cond_destroy(&empty_cv)) {
        perror2("pthread_cond_destroy2", err);
		// return 1;
	}
	if (err = pthread_cond_destroy(&full_cv)) {
        perror2("pthread_cond_destroy3", err);
		return 1;
	}
	if (err = pthread_mutex_destroy(&devices_mtx)) {
		perror2("pthread_mutex_destroy4", err);
		return 1;
	}
	if (err = pthread_cond_destroy(&allDone_cv)) {
		perror2("pthread_cond_destroy5", err);
		return 1;
	}
	if (err = pthread_mutex_destroy(&bytes_mtx)) {
		perror2("pthread_mutex_destroy6", err);
		return 1;
	}
	if (err = pthread_mutex_destroy(&managers_mtx)) {
		perror2("pthread_mutex_destroy7", err);
		return 1;
	}

	free(dir);
	close(sock);
	free(workers_tids);
	destroyBuffer(buffer);
	printf("Its been a pleasure working with you ! :)\n");
	return 0;
}
