#include "console_header.h"


int main(int argc, char *argv[]) {
	int flagO = 0, flagW = 0, flagR = 0; // Flags in order to check which operation characters have been put as input.
	char *jms_in, *jms_out;
	FILE *oper_fp;
	int write_fd, read_fd;

	if (argc != 5 && argc != 7) {
		fprintf(stderr, "Error ! Wrong input of arguments\n");
        return 1;
	}

	//A 'for' loop in order to check the in-line arguments.
	for (int i = 1; i < argc; i+= 2) {
		if ( strcmp(argv[i], "-w") == 0 ) {
            flagW = 1;
			jms_in = argv[i+1];
			if ((write_fd = open(jms_in, O_WRONLY | O_NONBLOCK)) < 0) {
				perror("Error ! Can't open jms_in!");
				return 1;
			}
        }
		else if ( strcmp(argv[i], "-r") == 0 ) {
			flagR = 1;
			jms_out = argv[i+1];
			if ((read_fd = open(jms_out, O_RDONLY | O_NONBLOCK)) < 0) {
				perror("Error ! Can't open jms_out!");
				return 1;
			}
		}
		else if ( strcmp(argv[i], "-o") == 0 ) {
			flagO = 1;
			if ((oper_fp = fopen(argv[i+1], "r")) == 0) {
                perror("Error ! Can't open operations file!");
                return 1;
            }
		}
		else {
			fprintf(stderr, "Error ! Unacceptable in-line argument!\n");
            return 1;
		}
	}

	if ( !flagW || !flagR) {
		fprintf(stderr, "Error ! Wrong input of arguments ! Missing a mandatory one !\n");
        return 1;
	}

	if ( !flagO ) {
		getInput(read_fd, write_fd, stdin);
	}
	else {
		if (getInput(read_fd, write_fd, oper_fp) != -1) { // no shutdown was read from the operations file
			printf("Reading from stdin now\n");
			getInput(read_fd, write_fd, stdin);
		}
		fclose(oper_fp);
	}



	close(write_fd);
	close(read_fd);
	return 0;
}
