#include "ContentHeader.h"



int main(int argc, char *argv[]) {
	struct stat path_stat = {0};
	int portNum, CSid, delay;
	char *dirOrfilename;
	int isDir = 0;
	int flagP, flagD; // Flags in order to check which operation characters have been put as input.
	flagD = flagP = 0;
	DelayList *list;

	if ( argc != 5 ) {
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
		}
		else if (strcmp(argv[i], "-d") == 0) {
			dirOrfilename = argv[i+1];
			if (stat(dirOrfilename, &path_stat) == -1) {
		    	perror("ContentServer dirOrfilename");
				return 2;
		    }
			flagD = 1;
		}
		else {
            fprintf(stderr, "Error ! Unacceptable in-line argument!\n");
            return 1;
        }
	}

	if ( !flagP || !flagD ) {
        fprintf(stderr, "Error ! Wrong input of arguments ! Missing a mandatory one !\n");
        return 1;
    }

	initializeDelayList(&list);

	int sock, readSock, bytes_in;
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
    if (listen(sock, 20) < 0) {
		perror("listen");
		return 1;
	}
    printf("Listening for connections to port %d\n", portNum);

	char buf[MSG_BUF], *token, *file;
	memset(buf, '\0', MSG_BUF);
	pthread_t thr;
	int err;
	while (1) {
		clientlen = sizeof(client);
		/* accept connection */
		if ((readSock = accept(sock, clientptr, &clientlen)) < 0) {
			perror("accept");
			return 1;
		}
		// printf("Accepted connection from MirrorManager/Worker\n");
		//Getting request from the MirrorManager or MirrorServer Workers.
		while (bytes_in = read(readSock, buf, MSG_BUF) <= 0) {
			//waiting to read something..
		}

		token = strtok(buf, " ");
		if (strcmp(token, "LIST") == 0) {
			token = strtok(NULL, " ");
			CSid = atoi(token);
			token = strtok(NULL, " ");
			delay = atoi(token);
			addDelayNodeToDelayList(list, CSid, delay);
			ListParams *lp;
			lp = (ListParams *)malloc(sizeof(ListParams));
			lp->sock = readSock;
			lp->dir = dirOrfilename;

			if (err = pthread_create(&thr, NULL, executingLIST, (void *)lp)) { /* New thread */
      			perror2("pthread_create", err);
      			return 1;
      		}
		}
		else if (strcmp(token, "FETCH") == 0) {
			token = strtok(NULL, " ");
			file = token;
			token = strtok(NULL, " ");
			CSid = atoi(token);
			delay = retDelayFromDelayList(list, CSid);
			FetchParams *fp;
			fp = (FetchParams *)malloc(sizeof(FetchParams));
			fp->sock = readSock;
			fp->dir = dirOrfilename;
			fp->requested = (char *)malloc(MSG_BUF*sizeof(char));
			sprintf(fp->requested, "%s", file);
			fp->delay = delay;

			if (err = pthread_create(&thr, NULL, executingFETCH, (void *)fp)) { /* New thread */
      			perror2("pthread_create", err);
      			return 1;
      		}
		}
	}
	close(sock);
	return 0;
}
