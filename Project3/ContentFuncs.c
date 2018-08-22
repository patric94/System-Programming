#include "ContentHeader.h"

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

void initializeDelayList(DelayList **list) {
	if (((*list) = (DelayList *)malloc(sizeof(DelayList))) == NULL) {
		fprintf(stderr, "DelayList malloc error!\n");
		return;
	}
	(*list)->first = NULL;
	(*list)->last = NULL;
}

void addDelayNodeToDelayList(DelayList *list, int id, int delay){
	DelayNode *dn;

	if ((dn = (DelayNode *)malloc(sizeof(DelayNode))) == NULL) {
		fprintf(stderr, "DelayNode malloc error!\n");
		return;
	}
	dn->id = id;
	dn->delay = delay;
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

int retDelayFromDelayList(DelayList *list, int id){
	DelayNode *dn;

	dn = list->first;
	while (dn !=NULL) {
		if (dn->id == id) {
			return dn->delay;
		}
		dn = dn->next;
	}
	return -1;
}

void destroyDelayList(DelayList *list){
	DelayNode *dn, *temp;
	dn = list->first;
	while (dn != NULL) {
		temp = dn;
		dn = dn->next;
		free(temp);
	}
	free(list);
}

void *executingLIST(void *arg) {
	ListParams *lp = (ListParams *) arg;
	printf("Executing LIST on dir %s\n", lp->dir);
	sendFilesFromList(lp->dir, lp->sock);

	close(lp->sock);
	free((ListParams *) arg);
	// printf("OK\n");
	pthread_exit(NULL);
}


void sendFilesFromList(char *dir, int sock){
	char buf[MSG_BUF];
	memset(buf, '\0', MSG_BUF);
	struct stat st = {0};

	// printf("%s\n", dir);
	stat(dir, &st);
	if (S_ISDIR(st.st_mode)) {
		chdir(dir);
    	show_dir_content(".", sock);
	}
	else if (S_ISREG(st.st_mode)) {
		sprintf(buf, "%s", dir);
		write(sock, buf, MSG_BUF);
	}
}

void show_dir_content(char *path, int sock){
	DIR *d;
	if ((d = opendir(path)) == NULL) {
		perror("opendir");
		return;
	}
	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type != DT_DIR) {
			char f_path[MSG_BUF];
			memset(f_path, '\0', MSG_BUF);
			// if (strcmp(path, ".") == 0) {
			// 	sprintf(f_path, "/%s", dir->d_name);
			// }
			// else {
			// }
			sprintf(f_path, "%s/%s", path, dir->d_name);

			if (write(sock, f_path, MSG_BUF) < 0) {
				perror("write");
			}
			memset(f_path, '\0', MSG_BUF);
		}
		else if (dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ) {
			char d_path[MSG_BUF];
			memset(d_path, '\0', MSG_BUF);
			sprintf(d_path, "%s/%s", path, dir->d_name);
			show_dir_content(d_path, sock);
		}
	}
	closedir(d);
}

void *executingFETCH(void *arg){
	FetchParams *fp = (FetchParams *)arg;
	printf("Executing FETCH on dir %s for file %s with delay %d\n", fp->dir, fp->requested, fp->delay);

	fetchFile(fp->dir, fp->requested, fp->sock, fp->delay);
	close(fp->sock);
	free((FetchParams *) arg);
	pthread_exit(NULL);
}

void fetchFile(char *dir, char *filePath, int sock, int delay){
	int fd, read_bytes, send_bytes;
	char *file_buf;

	if ((file_buf = (char *)malloc(512*sizeof(char))) == NULL) {
		perror("file_buf malloc");
	}

	chdir(dir);
	if ((fd = open(filePath, O_RDONLY)) < 0) {
		perror("poutsa");
	}

	sleep(delay);

	int totalBytes = 0;
	while ((read_bytes = read(fd, file_buf, 512)) > 0) {
		if ((send_bytes = send(sock, file_buf, read_bytes, 0)) < read_bytes) {
			perror("send error");
			return;
		}
		totalBytes += send_bytes;
	}
	printf("Send a file of %d Bytes\n", totalBytes);
	free(file_buf);
	close(fd);
}
