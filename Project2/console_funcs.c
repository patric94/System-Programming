#include "console_header.h"

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

int getInput(int read_fd, int write_fd, FILE *oper_fp){
	fd_set read_fds;
	int retval, bytes_in;
	char read_buf[MSG_BUF], write_buf[MSG_BUF];
	memset(read_buf, '\0', MSG_BUF);
	memset(write_buf, '\0', MSG_BUF);

	FD_ZERO(&read_fds);
	FD_SET(read_fd, &read_fds);

	int flagExit = 0;
	while (fgets(write_buf, sizeof(write_buf), oper_fp) != NULL) {
		char temp[MSG_BUF], *token;
		char *newline = strchr(write_buf, '\n' ); //getting rid of newline character
		*newline = 0;

		strcpy(temp, write_buf);
		token = strtok(temp," ");

		if (oper_fp != stdin) {
			printf("%s\n", write_buf);
		}
		write(write_fd, write_buf, MSG_BUF);
		if (strcmp(token, "shutdown") == 0) {
			flagExit = 1;
		}
		retval = select(read_fd +1, &read_fds, NULL, NULL, NULL);
		if ( retval == -1 ) {
			perror("select error");
			return -1;
		}
		else if ( retval ) {
			if (strcmp(token, "submit") == 0 || strcmp(token, "status") == 0 || strcmp(token, "suspend") == 0 || strcmp(token, "resume") == 0 || strcmp(token, "shutdown") == 0) {
				bytes_in = read(read_fd, read_buf, MSG_BUF);
				read_buf[bytes_in] = '\0';
				fprintf(stdout, "%s\n", read_buf);
			}
			else {
				int wait;
				while (1) {
					if ((bytes_in = read(read_fd, read_buf, MSG_BUF)) == 0) {
						continue;
					}
					read_buf[bytes_in] = '\0';
					if (strcmp(read_buf, "end_of_command") == 0) {
						break;
					}
					fprintf(stdout, "%s\n", read_buf);
					wait = select(read_fd +1, &read_fds, NULL, NULL, NULL);
					if ( wait == -1 ) {
						perror("select error");
						return -1;
					}
					else if (wait) {
						continue;
					}
				}
			}
		}
		if (flagExit) {
			return -1;
		}
	}
	return 0;
}
