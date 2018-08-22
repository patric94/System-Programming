#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "functions.h"


int main(int argc, char *argv[]) {

	int NumOfEntries1, NumOfEntries2, bucketSize, totalRecs;
	int flagN1, flagN2, flagB, flagO, flagC, retval;   // Flags in order to check which operation characters have been put as input.
    flagN1 = flagN2 = flagB = flagO = flagC = 0;
	Cost costs[5];
	FILE *oper_fp, *config_fp;

	if (argc != 9 && argc != 11) {
		fprintf(stderr,"Error ! Wrong input of arguments ! Please re-run the program correctly!\n");
		return EXIT_FAILURE;
	}

	for (int i = 1; i < argc; i+=2) {
		if (strcmp(argv[i], "-h1") == 0) {
            if(!isNumber(strlen(argv[i+1]), argv[i+1])){ //Using a fucntion to check is a string contains any non-arithmetic characters
                fprintf(stderr,"Error ! Hash table 1 size is not a number ! Please re-run the program correctly!\n");
                return EXIT_FAILURE;
            }
            flagN1 = 1;
            NumOfEntries1 = atoi(argv[i+1]);
        }
		else if (strcmp(argv[i], "-h2") == 0) {
			if(!isNumber(strlen(argv[i+1]), argv[i+1])){ //Using a fucntion to check is a string contains any non-arithmetic characters
                fprintf(stderr,"Error ! Hash table 2 size is not a number ! Please re-run the program correctly!\n");
                return EXIT_FAILURE;
            }
            flagN2 = 1;
            NumOfEntries2 = atoi(argv[i+1]);
		}
		else if (strcmp(argv[i], "-s") == 0) {
			//I use the same bucket size for both blue and pink bucket.
			if(!isNumber(strlen(argv[i+1]), argv[i+1])){ //Using a fucntion to check is a string contains any non-arithmetic characters
                fprintf(stderr,"Error ! Bucket size is not a number ! Please re-run the program correctly!\n");
                return EXIT_FAILURE;
            }
            flagB = 1;
            bucketSize = atoi(argv[i+1]);
		}
		else if (strcmp(argv[i], "-o") == 0) {
			flagO = 1;
            if ((oper_fp = fopen(argv[i+1], "r")) == 0) {
                fprintf(stderr,"Error ! Can't open operation file! \n");
                return EXIT_FAILURE;
            }
		}
		else if (strcmp(argv[i], "-c") == 0) {
			flagC = 1;
            if ((config_fp = fopen(argv[i+1], "r")) == 0) {
                fprintf(stderr,"Error ! Can't open configuration file! \n");
                return EXIT_FAILURE;
            }
			char buffer[200];
			int i = 0;
			while(fgets(buffer,sizeof(buffer),config_fp) != NULL){
				char* token;
		        char* newline = strchr(buffer, '\n' ); //getting rid of newline character
		        *newline = 0;
				token = strtok(buffer,";");
				costs[i].type = atoi(token);
				token = strtok(NULL,";");
				costs[i].tariff = atoi(token);
				token = strtok(NULL,";");
				costs[i].cost = atof(token);
				i++;
			}
		}
		else {
            fprintf(stderr,"Error ! Not an acceptable parameter ! Please re-run the program correctly!\n");
            return EXIT_FAILURE;
        }
	}
	if (!flagB || !flagN1 || !flagN2 || !flagC) {
		fprintf(stderr, "A mandatory parameter is missing ! PLease re-run the program correctly!\n");
		return EXIT_FAILURE;
	}
	if (!flagO) {
		oper_fp = stdin;
	}

	totalRecs = bucketSize / sizeof(CDR);
	printf("bucketSize = %d, CDRsize = %d, totalRecs per Bucket = %d\n", bucketSize, sizeof(CDR), totalRecs);

	hashBucket **caller_table;
	hashBucket **callee_table;
	initHashTable(&caller_table, NumOfEntries1, totalRecs);
	initHashTable(&callee_table, NumOfEntries2, totalRecs);

	MaxHeap	*heap;
	initMaxHeap(&heap);

	fflush(stderr);
	fflush(stdout);

	if ((retval = getInput(oper_fp, NumOfEntries1, NumOfEntries2, bucketSize, totalRecs, costs, caller_table, callee_table, heap)) < 0) {
		return EXIT_FAILURE;
	}
	else if (retval && flagO) {
		// printf("---------\n");
		// printMaxHeap(heap->root);
		// printf("---------\n");
		freeHash(caller_table, NumOfEntries1, totalRecs, 0);
		freeHash(callee_table, NumOfEntries2, totalRecs, 1);
		freeSubTree(heap->root);
		free(heap);

		initHashTable(&caller_table, NumOfEntries1, totalRecs);
		initHashTable(&callee_table, NumOfEntries2, totalRecs);
		initMaxHeap(&heap);
		if ((retval = getInput(stdin, NumOfEntries1, NumOfEntries2, bucketSize, totalRecs, costs, caller_table, callee_table, heap)) < 0) {
			return EXIT_FAILURE;
		}
		freeHash(caller_table, NumOfEntries1, totalRecs, 0);
		freeHash(callee_table, NumOfEntries2, totalRecs, 1);
		if (heap->nodes) {
			freeSubTree(heap->root);
		}
		free(heap);
	}
	else {//either exit or end of file
		// printf("---------\n");
		// printMaxHeap(heap->root);
		// printf("---------\n");
		//
		// float max = getMax(heap);
		// printf("%.2f \n", max);
		freeHash(caller_table, NumOfEntries1, totalRecs, 0);
		freeHash(callee_table, NumOfEntries2, totalRecs, 1);
		freeSubTree(heap->root);
		free(heap);
	}
	fclose(config_fp);
	if(flagO){
        fclose(oper_fp);
    }
	return 0;
}
