#include "InitiatorHeader.h"

char *end;

int main(int argc, char *argv[]) {
	int portNum, ContentServersRequests = 0;
	char *address;
	int flagP, flagN, flagS; // Flags in order to check which operation characters have been put as input.
	flagN = flagP = flagS = 0;
	char **dataToServer = NULL;
	char buf[MSG_BUF];
	memset(buf, '\0', MSG_BUF);

	if ( argc < 7 ) {
        fprintf(stderr, "Error ! Wrong input of arguments\n");
        return 1;
    }
	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-n") == 0) {
			if (flagN) {
				fprintf(stderr, "Error ! Flag %s has already been inserted !\n", argv[i]);
				return 1;
			}
			i++;
			address = argv[i];
			flagN = 1;
			i++;
		}
		else if (strcmp(argv[i], "-p") == 0) {
			if (flagP) {
				fprintf(stderr, "Error ! Flag %s has already been inserted !\n", argv[i]);
				return 1;
			}
			i++;
			if ( !isNumber(strlen(argv[i]), argv[i]) ) {
                fprintf(stderr, "Error ! Port Number must be an integer value !\n");
                return 1;
            }
			flagP = 1;
			portNum = atoi(argv[i]);
			i++;
		}
		else if (strcmp(argv[i], "-s") == 0) {
			if (flagS) {
				fprintf(stderr, "Error ! Flag %s has already been inserted !\n", argv[i]);
				return 1;
			}
			i++;
			flagS = 1;
			char *token;
			while (i < argc && strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-n") != 0) {
				ContentServersRequests ++;
				if ((dataToServer = (char **)realloc(dataToServer,ContentServersRequests*sizeof(char*))) == NULL) {
					free(dataToServer);
					perror("Error (re)allocating memory");
					return 1;
				}
				dataToServer[ContentServersRequests-1] = argv[i];
				i++;
			}
		}
		else {
            fprintf(stderr, "Error ! Unacceptable in-line argument!\n");
            return 1;
        }
	}
	if ( !flagP || !flagN || !flagS ) {
        fprintf(stderr, "Error ! Wrong input of arguments ! Missing a mandatory one !\n");
        return 1;
    }

	int sock = make_conn_socket(portNum, address);

    printf("Connecting to %s port %d\n", address, portNum);
	int byteswr;
	for (int i = 0; i < ContentServersRequests; i++) {
		strcat(buf, dataToServer[i]);
		if ((byteswr = write(sock, buf, MSG_BUF) < 0)){
			perror("write to sock");
			return 1;
		}
		memset(buf, '\0', MSG_BUF);
	}
	sprintf(buf, "end_of_command");
	write(sock, buf, MSG_BUF);
	memset(buf, '\0', MSG_BUF);

	printf("Send data to MirrorServer!\n");

	read(sock, buf, MSG_BUF);
	printf("%s\n", buf);
	free(dataToServer);
	return 0;
}
