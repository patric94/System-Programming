#include "ServerHeader.h"

extern int ContentServersRequests;
extern int started, working;
extern char **ServerAdd, **dirOrfiles, **dirNames;
extern int *delays, *ServerPort;
extern int endOfExecution;
extern int bytesTransferred, filesTransferred, numWorkersDone, managersDone;
extern FileList *flist;
extern Buffer *buffer;
extern pthread_mutex_t buffer_mtx, devices_mtx, bytes_mtx, managers_mtx;
extern pthread_cond_t empty_cv, full_cv, allDone_cv;

void terminate(int sig) {
	working = 0;
	printf("Gonna terminate now!\n");
}

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

int initializeBuffer(Buffer **buf, int size){
	if (((*buf) = (Buffer *)malloc(sizeof(Buffer))) == NULL) {
		fprintf(stderr, "Buffer mallock error !\n");
		return -1;
	}
	if (((*buf)->data = (char **)malloc(size*sizeof(char *))) == NULL) {
		perror("Buffer malloc");
		return -1;
	}
	for (int i = 0; i < size; i++) {
		if (((*buf)->data[i] = (char *)malloc((MSG_BUF*2)*sizeof(char))) == NULL) {
			perror("Buffer->data[i] malloc");
			return -1;
		}
		memset((*buf)->data[i], '\0', MSG_BUF);
	}
	(*buf)->size = size;
	(*buf)->count = 0;
	return 0;
}

void destroyBuffer(Buffer *buf){
	for (int i = 0; i < buf->size; i++) {
		free(buf->data[i]);
	}
	free(buf->data);
	free(buf);
}

void initializeDirList(DirList **list){
	if (((*list) = (DirList *)malloc(sizeof(DirList))) == NULL) {
		fprintf(stderr, "DirList malloc error!\n");
		return;
	}
	(*list)->first = NULL;
	(*list)->last = NULL;
}

void addDirNodeToDirList(DirList *list, char *cont){
	DirNode *dn;
	if ((dn = (DirNode *)malloc(sizeof(DirNode))) == NULL) {
		fprintf(stderr, "DirNode malloc error!\n");
		return;
	}
	if ((dn->cont = (char *)malloc(MSG_BUF*sizeof(char))) == NULL) {
		perror("dn->cont malloc");
		return;
	}
	memset(dn->cont, '\0', MSG_BUF);
	sprintf(dn->cont, "%s", cont);
	dn->next = NULL;

	if (list->first == NULL) { //if list is empty.
		list->first = dn;
		list->last = dn;
	}
	else { //if list contains at least one node.
		list->last->next = dn;
		list->last = dn;
	}
}

void printDirList(DirList *list){
	DirNode *temp;
	temp = list->first;
	printf("Printing DirList\n");
	while (temp != NULL) {
		printf("%s\n", temp->cont);
		temp = temp->next;
	}
}

void destroyDirList(DirList *list) {
	DirNode *dn, *temp;
	dn = list->first;
	while (dn != NULL) {
		temp = dn;
		dn = dn->next;
		free(temp->cont);
		free(temp);
	}
	free(list);
}

void initializeFileList(FileList **list){
	if (((*list) = (FileList *)malloc(sizeof(FileList))) == NULL) {
		fprintf(stderr, "FileList malloc error!\n");
		return;
	}
	(*list)->first = NULL;
	(*list)->last = NULL;
}

void addFileNodeToFileList(FileList *list, int bytes){
	FileNode *fn;
	if ((fn = (FileNode *)malloc(sizeof(FileNode))) == NULL) {
		fprintf(stderr, "FileNode malloc error!\n");
		return;
	}
	fn->bytes = bytes;
	fn->next = NULL;

	if (list->first == NULL) { //if list is empty.
		list->first = fn;
		list->last = fn;
	}
	else { //if list contains at least one node.
		list->last->next = fn;
		list->last = fn;
	}
}

double getDispersion(FileList *list, double avg, int totalFiles){
	FileNode *fn;
	double spread = 0;
	fn = list->first;
	while (fn != NULL) {
		double diff = (double)fn->bytes - avg;
		spread += pow(diff, 2);
		fn = fn->next;
	}
	spread /= totalFiles;
	return spread;
}

void destroyFileList(FileList *list){
	FileNode *fn, *temp;
	fn = list->first;
	while (fn != NULL) {
		temp = fn;
		fn = fn->next;
		free(temp);
	}
	free(list);
}

int make_conn_socket(int port, char *address){
	int sock;
	struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
	/* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		return -1;
	}
	/* Find Server's address */
    if ((rem = gethostbyname(address)) == NULL) {
	   herror("gethostbyname");
	   return -1;
    }
	server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);         /* Server port */

	/* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0){
		perror("connect socket");
		return -1;
	}
	return sock;
}

int make_accept_socket(int *sock, int *readSock, int port){
	struct sockaddr_in server, client;
    socklen_t clientlen;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;

	/* Create socket */
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);      /* The given port */
    /* Bind socket to address */
    if (bind(*sock, serverptr, sizeof(server)) < 0) {
		perror("bind");
		return -1;
	}
	/* Listen for connections */
    if (listen(*sock, 5) < 0) {
		perror("listen");
		return -1;
	}
    printf("Listening for connections to port %d\n", port);
	clientlen = sizeof(client);
	/* accept connection */
	if ((*readSock = accept(*sock, clientptr, &clientlen)) < 0) {
		perror("accept");
		return -1;
	}
	printf("Accepted connection from MirrorInitiator\n");
}

int getInitiatorInfo(int readSock, char *dir) {
	int wait;
	char buf[MSG_BUF];
	memset(buf, '\0', MSG_BUF);
	int bytes_in;
	char *token;
	while (1) {
		if ((bytes_in = read(readSock, buf, MSG_BUF)) == 0) {
			continue;
		}
		buf[bytes_in] = '\0';
		if (strcmp(buf, "end_of_command") == 0) {
			break;
		}
		ContentServersRequests++;
		token = strtok(buf, ":");
		if ((ServerAdd = (char **)realloc(ServerAdd, ContentServersRequests*sizeof(char*))) == NULL) {
		    free(ServerAdd);
		    perror("Error (re)allocating memory");
		    return -1;
		}
		if ((ServerAdd[ContentServersRequests-1] = (char *)malloc(strlen(token)*sizeof(char)+1)) == NULL) {
			perror("malloc");
			return -1;
		}
		strcpy(ServerAdd[ContentServersRequests-1], token);
		token = strtok(NULL, ":");
		if ((ServerPort = (int *)realloc(ServerPort, ContentServersRequests*sizeof(int))) == NULL) {
		    free(ServerPort);
		    perror("Error (re)allocating memory");
		    return -1;
		}
		ServerPort[ContentServersRequests-1] = atoi(token);
		token = strtok(NULL, ":");
		if ((dirOrfiles = (char **)realloc(dirOrfiles, ContentServersRequests*sizeof(char*))) == NULL) {
		    free(dirOrfiles);
		    perror("Error (re)allocating memory");
		    return -1;
		}
		if ((dirOrfiles[ContentServersRequests-1] = (char *)malloc(strlen(token)*sizeof(char)+1)) == NULL) {
			perror("malloc");
			return -1;
		}
		strcpy(dirOrfiles[ContentServersRequests-1], token);
		token = strtok(NULL, ":");
		if ((delays = (int *)realloc(delays, ContentServersRequests*sizeof(int))) == NULL) {
		    free(delays);
		    perror("Error (re)allocating memory");
		    return -1;
		}
		delays[ContentServersRequests-1] = atoi(token);
		chdir(dir);
		if ((dirNames = (char **)realloc(dirNames, ContentServersRequests*sizeof(char*))) == NULL) {
		    free(dirNames);
		    perror("Error (re)allocating memory");
		    return -1;
		}
		if ((dirNames[ContentServersRequests-1] = (char *)malloc(MSG_BUF*sizeof(char))) == NULL) {
			perror("malloc");
			return -1;
		}
		sprintf(dirNames[ContentServersRequests-1], "%s_%d_%d", ServerAdd[ContentServersRequests-1], ServerPort[ContentServersRequests-1], ContentServersRequests-1);
		if (mkdir(dirNames[ContentServersRequests-1], 0700) != 0) {
			perror("req mkdir error");
			return 1;
		}
		chdir("..");
	}
	return 0;
}

void *MirrorManager(void *arg){
	char buf[MSG_BUF];
	memset(buf, '\0', MSG_BUF);
	int contServer = *((int *)arg);
	DirList *list;
	DirNode *dn;

	initializeDirList(&list);

	int sock = make_conn_socket(ServerPort[contServer], ServerAdd[contServer]);
	if (sock < 0) {
		fprintf(stderr, "Error in Thread %ld \n", pthread_self());
		pthread_exit(NULL);
	}
	printf("\tContServer %d\n", contServer);
    printf("\tConnecting to %s port %d\n", ServerAdd[contServer], ServerPort[contServer]);

	sprintf(buf, "LIST %d %d", contServer, delays[contServer]);
	write(sock, buf, MSG_BUF);
	memset(buf, '\0', MSG_BUF);
	while (read(sock, buf, MSG_BUF) > 0) {
		addDirNodeToDirList(list, buf);
	}
	close(sock);
	// printDirList(list);
	if (!placeInBuffer(list, contServer)) { //Didnt find the requested folder/file.
		printf("Didnt find the requested Directory/File %s\n", dirOrfiles[contServer]);
	}

	printf("\tManager of ContentServersRequest %d finished\n", contServer);
	pthread_mutex_lock(&managers_mtx);
	managersDone++;
	pthread_mutex_unlock(&managers_mtx);
	destroyDirList(list);
	free((int *) arg);
	pthread_exit(NULL);
}

int placeInBuffer(DirList *list, int contID) {
	DirNode *dn;
	dn = list->first;
	int Found = 0;
	while (dn != NULL) {
		// printf("%s\n", dn->cont);
		// if (strstr(dn->cont, dirOrfiles[contID]) != NULL) {
		if (validPath(dn->cont, dirOrfiles[contID])) { //The file in the DirList is in the directory that Initiator asked for
			// printf("\tFound one %s\n", dn->cont);
			Found = 1;
			pthread_mutex_lock(&buffer_mtx);
			//we check if buffer is full
			while (buffer->count == buffer->size) {
				printf("\tFound buffer full\n");
				pthread_cond_wait(&full_cv, &buffer_mtx);
			}
			printf("\tGonna place it in %d cell\n", buffer->count);
			//we can place things in the buffer
			sprintf(buffer->data[buffer->count], "%s,%s,%d,%d", dn->cont, ServerAdd[contID], ServerPort[contID], contID);
			buffer->count++;

			//unlocking the mutex! we're done with the CS here
			pthread_cond_broadcast(&empty_cv);
			pthread_mutex_unlock(&buffer_mtx);

		}
		dn = dn->next;
	}
	return Found;
}

int validPath(char *dnContent, char *InitiatorContent){
	//This funcion checks if the file in the LIST from ContentServer exists in the specific
	//hierarchy that the MirrorInitiator asked for.
	char *dnTok, *initTok;
	char *tempDN = NULL, *tempIC = NULL;
	char *tempDNptr, *tempICptr;

	if ((tempDN = (char *)malloc(strlen(dnContent)*sizeof(char)+1)) == NULL) {
		perror("validPath malloc");
		return 0;
	}
	strcpy(tempDN, dnContent);
	tempDNptr = tempDN;
	if ((tempIC = (char *)malloc(strlen(InitiatorContent)*sizeof(char)+1)) == NULL) {
		perror("validPath malloc");
		return 0;
	}
	strcpy(tempIC, InitiatorContent);
	tempICptr = tempIC;
	// printf("OK %s %s\n", tempDN, tempIC);
	if (strstr(tempDN, tempIC) != NULL) {
		dnTok = strtok_r(tempDNptr, "./", &tempDNptr);
		initTok = strtok_r(tempICptr, "./", &tempICptr);
		// printf("OK %s %s\n", dnTok, initTok);
		while (dnTok != NULL && initTok != NULL) {
			if (strcmp(dnTok, initTok)) {
				free(tempDN);
				free(tempIC);
				return 0;
			}
			dnTok = strtok_r(tempDNptr, "/", &tempDNptr);
			initTok = strtok_r(tempICptr, "/", &tempICptr);
		}
		free(tempDN);
		free(tempIC);
		return 1;
	}
	else {
		free(tempDN);
		free(tempIC);
		return 0;
	}

}

void *Worker(void * arg) {
	char *dir = (char *) arg;
	int done = 0;
	// printf("\t\tWorker %ld started\n", pthread_self());
	char *workerBuf;
	while (1) {
		if ((workerBuf = (char *)malloc(2*MSG_BUF*sizeof(char))) == NULL) {
			perror("workerBuf malloc");
		}
		memset(workerBuf, '\0', 2*MSG_BUF);
		pthread_mutex_lock(&buffer_mtx);

		//we check if buffer is empty
		while (buffer->count == 0 && endOfExecution == 0) {
			pthread_mutex_lock(&managers_mtx);
			if (managersDone == ContentServersRequests && started) {
				pthread_mutex_lock(&devices_mtx);
				numWorkersDone++;
				pthread_mutex_unlock(&devices_mtx);
				pthread_cond_broadcast(&allDone_cv);
			}
			pthread_mutex_unlock(&managers_mtx);
			printf("\t\tFound buffer empty %ld\n", pthread_self());
			pthread_cond_wait(&empty_cv, &buffer_mtx);
			if (endOfExecution) {
				// pthread_cond_broadcast(&empty_cv);
				break;
			}
		}
		if (endOfExecution) {
			free(workerBuf);
			break;
		}
		//we can get things from the buffer
		printf("\t\tBuffer not empty gonna take smthing from %d\n", buffer->count-1);
		// printf("Worker %ld read : %s\n", pthread_self(), buffer->data[buffer->count-1]);
		sprintf(workerBuf, "%s", buffer->data[buffer->count-1]);
		// memset(buffer->data[buffer->count-1], '\0', 2*MSG_BUF);
		buffer->count--;
		//unlocking the mutex! we're done with the CS here
		pthread_cond_broadcast(&full_cv);
		pthread_mutex_unlock(&buffer_mtx);

		getFileFromContentServer(workerBuf, dir);
		free(workerBuf);
	}

	// free((int *) arg);
	pthread_exit(NULL);
}

void getFileFromContentServer(char *buf, char *dir) {
	int sock;
	int fd;
	char *file, *address, *tempbuf, *file_buf;
	char *tokedFile, *tokedFileptr;
	char *token, *curr_tok, *next_tok;
	char *fileLocation;
	int port, contID, rcvd_bytes, totalBytes = 0;
	struct stat st = {0};

	fileLocation = (char *)malloc(1024*sizeof(char));
	memset(fileLocation, '\0', 1024);

	tempbuf = (char *)malloc(MSG_BUF*sizeof(char));
	memset(tempbuf, '\0', MSG_BUF);

	token = strtok(buf, ",");
	file = token;
	token = strtok(NULL, ",");
	address = token;
	token = strtok(NULL, ",");
	port = atoi(token);
	token = strtok(NULL, ",");
	contID = atoi(token);

	sock = make_conn_socket(port, address);
	if (sock < 0) {
		fprintf(stderr, "Error in Thread %ld \n", pthread_self());
		pthread_exit(NULL);
	}
	printf("\t\tConnecting to %s port %d\n", address, port);
	sprintf(tempbuf, "FETCH %s %d", file, contID);
	write(sock, tempbuf, MSG_BUF);

	sprintf(fileLocation, "%s%s", dir, dirNames[contID]);

	if ((tokedFile = (char *)malloc(strlen(file)*sizeof(char)+1)) == NULL) {
		perror("tokedFile malloc error");
		return;
	}
	sprintf(tokedFile, "%s", file);
	tokedFileptr = tokedFile;

	curr_tok = strtok_r(tokedFileptr, "./", &tokedFileptr);//to remove the '.' from the filepath

	while (curr_tok != NULL) {
		next_tok = strtok_r(tokedFileptr, "/", &tokedFileptr);
		strcat(fileLocation, "/");
		strcat(fileLocation, curr_tok);
		if (next_tok != NULL) {
			//The curr_tok doesnt point to the name of the file we wish to create
			if (stat(fileLocation, &st) < 0) { //if the dir isnt created.
				//Will create the directory
				if (mkdir(fileLocation, 0700) != 0) {
					// perror("fileLocation mkdir err");
					// return;
				}
			}
		}
		else {
			//create the file requested
			if ((file_buf = (char *)malloc(512*sizeof(char))) == NULL) {
				perror("file_buf malloc");
			}
			if ((fd = open(fileLocation, O_WRONLY | O_CREAT, 0644)) < 0) {
				printf("%s\n", fileLocation);
				perror("error creating file");
				return;
			}
		}
		curr_tok = next_tok;
	}

	while ((rcvd_bytes = recv(sock, file_buf, 512, 0)) > 0) {
		totalBytes += rcvd_bytes;
		if (write(fd, file_buf, rcvd_bytes) < 0) {
			perror("error writing to file");
			return;
		}
	}
	pthread_mutex_lock(&buffer_mtx);
	addFileNodeToFileList(flist, totalBytes);
	bytesTransferred += totalBytes;
	filesTransferred++;
	pthread_mutex_unlock(&buffer_mtx);

	printf("\t\tCopied the file of %d Bytes\n", totalBytes);
	close(fd);

	free(fileLocation);
	free(file_buf);
	free(tokedFile);
	close(sock);
	free(tempbuf);
}
