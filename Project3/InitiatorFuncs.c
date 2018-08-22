#include "InitiatorHeader.h"

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
