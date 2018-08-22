#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "functions.h"

int isNumber(int len, char *string){       //A fucntion to check if a given string is either an int or a float number.
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

int compareDates(char *dateFrom, char *dateTo, char *dateNow) {
	char day[3], month[3], year[5];
	int dateF, dF, mF, yF, dateT, dT, mT, yT, dateN, dN, mN, yN;
	memset(day, '\0', sizeof(day));
	memset(month, '\0', sizeof(month));
	memset(year, '\0', sizeof(year));

	strncpy(day, dateFrom, 2);
	strncpy(month, dateFrom+2, 2);
	strncpy(year, dateFrom+4, 4);
	dF = atoi(day);
	mF = atoi(month);
	yF = atoi(year);

	strncpy(day, dateTo, 2);
	strncpy(month, dateTo+2, 2);
	strncpy(year, dateTo+4, 4);
	dT = atoi(day);
	mT = atoi(month);
	yT = atoi(year);

	strncpy(day, dateNow, 2);
	strncpy(month, dateNow+2, 2);
	strncpy(year, dateNow+4, 4);
	dN = atoi(day);
	mN = atoi(month);
	yN = atoi(year);

	dateF = 10000 * yF + 100 * mF + dF;
	dateT = 10000 * yT + 100 * mT + dT;
	dateN = 10000 * yN + 100 * mN + dN;

	if (dateF <= dateN && dateN <= dateT) {
		return 1;
	}
	return 0;

}

int count_occur(char **table, char *map, int amount, int start){
	int count = 0;
	char value[4];

	memset(value, '\0', sizeof(value));
	strcpy(value, table[start]);

	for (int i = start; i < amount; i++) {
        if (strcmp(table[i], value) == 0){
            map[i] = 1;
            ++count;
        }
    }
    return count;
}

void countCountries(char **table, int amount){
	int num_occ, max = 0, maxi;
	char map[amount];
	memset(map, '\0', sizeof(map));

	for (int i = 0; i < amount; i++) {
		if (map[i] == 0) {
            num_occ = count_occur(table, map, amount, i);
			if (num_occ > max) {
				maxi = i;
				max = num_occ;
			}
        }
	}
	printf("Country code : %s, Calls made : %d\n", table[maxi], max);;
}

int findHash(hashBucket **table, char *caller, char *dateFrom, char *dateTo, char *timeFrom, char *timeTo, int NumOfEntries, int totalRecs){
	Record *aux;
	cdrBucket *iter;
	int found = 0;

	if ((aux = searchHash(table, caller, NumOfEntries, totalRecs)) != NULL) {
		iter = aux->header;
		while (iter != NULL) {
			for (int z = 0; z < totalRecs; z++) {
				if (iter->table[z] == NULL) {
					continue;
				}
				if ( dateFrom == NULL && dateTo == NULL && timeFrom == NULL && timeTo == NULL) {
					found++;
					printCDR(iter->table[z], stdout);
				}
				else if (dateFrom == NULL && dateTo == NULL) {
					if (strcmp(timeFrom, iter->table[z]->init_time) <= 0 && strcmp(iter->table[z]->init_time, timeTo) <= 0) {
						found++;
						printCDR(iter->table[z], stdout);
					}
				}
				else if (timeFrom == NULL && timeTo == NULL) {
					if (compareDates(dateFrom, dateTo, iter->table[z]->date)) {
						found++;
						printCDR(iter->table[z], stdout);
					}
				}
				else {
					if ((strcmp(timeFrom, iter->table[z]->init_time) <= 0 && strcmp(iter->table[z]->init_time, timeTo) <= 0) && compareDates(dateFrom, dateTo, iter->table[z]->date)) {
						found++;
						printCDR(iter->table[z], stdout);
					}
				}
			}
			iter = iter->next;
		}
		if (!found) {
			fprintf(stderr, "No CDRs found!\n");
		}
	}
	else {
		printf("Caller : %s wasnt found!\n", caller);
	}
	return 0;
}

void contactedWith(hashBucket **table, indistList *list, char *caller, int NumOfEntries, int totalRecs, int mode){
	hashBucket *temp;
	Record *aux;
	cdrBucket *info;
	indistNode *iter, *node;

	for (int i = 0; i < NumOfEntries; i++) {
		temp = table[i];
		while (temp != NULL) {
			for (int j = 0; j < temp->items; j++) {
				aux = temp->table[j];
				if (strcmp(aux->number, caller) == 0) {
					continue;
				}
				info = aux->header;
				iter = list->header;
				int exists = 0;
				while (iter != NULL) {
					if (strcmp(iter->number, aux->number) == 0) {
						exists = 1;
						break;
					}
					iter = iter->next;
				}
				if (!exists) {
					while (info != NULL) {
						int placed = 0;
						for (int z = 0; z < totalRecs; z++) {
							if (info->table[z] == NULL) {
								continue;
							}
							if (mode) { //caller table
								if (!strcmp(info->table[z]->destination_number, caller)) {
									continue;
								}
							}
							else { //callee table
								if (!strcmp(info->table[z]->originator_number, caller)) {
									continue;
								}
							}
							node = createListNode(aux->number);
							list->count++;
							if (list->header == NULL) {
								list->header = node;
								list->last = node;
							}
							else {
								list->last->next = node;
								list->last = node;
							}
							placed = 1;
							break;
						}
						if (placed) {
							break;
						}
						info = info->next;
					}
				}
				else {
					continue;
				}
			}
			temp = temp->next;
		}
	}
}

indistList* sectionOfLists(indistList *list1, indistList *list2) {
	int ret;
	indistList *section;
	indistNode *exterior, *temp, *interior, *node;

	if (list1->count < list2->count) {
		exterior = list1->header;
		interior = list2->header;
		ret = 0;
	}
	else {
		exterior = list2->header;
		interior = list1->header;
		ret = 1;
	}

	createList(&section);
	while (exterior != NULL) {
		temp = interior;
		while (temp != NULL) {
			if (strcmp(temp->number, exterior->number) == 0) { //den exei iparxei epikoinonia metaksi kai ton dio callers.
				node = createListNode(exterior->number);
				section->count++;
				if (section->header == NULL) {
					section->header = node;
					section->last = node;
				}
				else {
					section->last->next = node;
					section->last = node;
				}
				break;
			}
			temp = temp->next;
		}
		exterior = exterior->next;
	}
	return section;
}

void diffCallers(hashBucket **table, indistList *list, int NumOfEntries, int totalRecs){
	int hash;
	indistNode *iter, *node;
	hashBucket *temp;
	Record *aux;
	cdrBucket *pars;

	iter = list->header;
	while (iter != NULL) { //enan enan tous arithmous mesa sto list
		if (iter->number != NULL) {
			hash = HashFunct(iter->number, NumOfEntries);
			temp = table[hash];
			while (temp != NULL) {
				for (int i = 0; i < temp->items; i++) {
					aux = temp->table[i];
					if (strcmp(aux->number, iter->number) == 0) { //otan ton bro koitao ta cdrs tou
						pars = aux->header;
						while (pars != NULL) {
							for (int j = 0; j < totalRecs; j++) {
								if (pars->table[j] == NULL) {
									continue;
								}
								node = list->header;
								while (node != NULL) {
									if (node->number != NULL) {
										if (strcmp(node->number, iter->number) != 0	) { // an den koitao ton idio arithmo apo ti lista
											if (strcmp(node->number, pars->table[j]->originator_number) == 0) {
												 memset(node->number, '\0', sizeof(node->number));
											}
										}
									}
									node = node->next;
								}
							}
							pars = pars->next;
						}
					}
				}
				temp = temp->next;
			}
		}
		iter = iter->next;
	}

}

int indist(hashBucket **caller_table, hashBucket **callee_table, char *caller1, char *caller2, int NumOfEntries1, int NumOfEntries2, int totalRecs) {
	Record *aux1, *aux2;
	cdrBucket *iter1, *iter2;
	indistList *list1, *list2, *section;

	if (searchHash(caller_table, caller1, NumOfEntries1, totalRecs) == NULL && searchHash(caller_table, caller2, NumOfEntries2, totalRecs) == NULL) {
		return -1;
	}

	createList(&list1);
	contactedWith(caller_table, list1, caller1, NumOfEntries1, totalRecs, 1);
	contactedWith(callee_table, list1, caller1, NumOfEntries2, totalRecs, 0);

	createList(&list2);
	contactedWith(caller_table, list2, caller2, NumOfEntries1, totalRecs, 1);
	contactedWith(callee_table, list2, caller2, NumOfEntries2, totalRecs, 0);

	section = sectionOfLists(list1, list2);
	diffCallers(callee_table, section, NumOfEntries2, totalRecs);

	printList(section);
	freeList(section);
	freeList(list1);
	freeList(list2);
	return 0;
}

int topDest(hashBucket **table, char *caller, int NumOfEntries, int totalRecs){
	Record *aux;
	cdrBucket *iter;
	char **temp;
	int i = 0, max;

	if ((aux = searchHash(table, caller, NumOfEntries, totalRecs)) != NULL) {
		iter = aux->header;
		if ((temp = (char **)malloc(aux->CDRamount*sizeof(char*))) == NULL) {
			fprintf(stderr, "Memory Error for table in topDest fucntion!\n");
			return -1;
		}
		for (int j = 0; j < aux->CDRamount; j++) {
			if ((temp[j] = (char *)malloc(4*sizeof(char))) == NULL) {
				fprintf(stderr, "Memory Error for table in topDest fucntion!\n");
				return -1;
			}
			memset(temp[j], '\0', 4);
		}

		while (iter != NULL) {
			for (int z = 0; z < totalRecs; z++) {
				if (iter->table[z] == NULL) {
					continue;
				}
				memcpy(temp[i], iter->table[z]->destination_number, 3);
				i++;
			}
			iter = iter->next;
		}

		countCountries(temp, aux->CDRamount);

		for (int k = 0; k < aux->CDRamount; k++) {
			free(temp[k]);
		}
		free(temp);
	}
	else {
		printf("Caller : %s wasnt found!\n", caller);
	}
	return 0;
}

void topK(MaxHeap *heap, int percent) {
	float max, sum, per, total = 0;
	char num[15];
	memset(num, '\0', sizeof(num));
	HeapNode *last, *parent;
	HeapList *list;
	HeapListNode *ln;

	createHeapList(&list);

	per = heap->companysIncome * ((float)percent/100);

	// printf("%.2f kai thelo to %.2f\n", heap->companysIncome, per);
	while (total < per) {
		last = findLastNode(heap);

		max = heap->root->sumOfMoney;
		strcpy(num, heap->root->number);
		sum = heap->root->sumOfMoney;
		strcpy(heap->root->number, last->number);
		heap->root->sumOfMoney = last->sumOfMoney;
		strcpy(last->number, num);
		last->sumOfMoney = sum;
		parent = last->parent;

		if (parent->left == last) {
			parent->left = NULL;
		}
		else {
			parent->right = NULL;
		}

		if (list->header == NULL) {
			list->header = createHeapListNode();
			list->header->hn = last;
			list->bottom = list->header;
		}
		else 	{
			list->bottom->next = createHeapListNode();
			list->bottom->next->hn = last;
			list->bottom = list->bottom->next;
		}

		printf("Caller %s with Money %.2f\n", last->number, last->sumOfMoney);
		heap->nodes--;
		heapify(heap->root);

		total += max;
	}

	ln = list->header;
	while (ln != NULL) {
		heap->nodes++;
		int len;
		int *bitString = createBitString(heap->nodes, &len);
		HeapNode *temp = heap->root, *par;
		for (int i = 0; i < len-1; i++) {
			if (i == len-2) {
				if (bitString[i]) {
					par = temp;
					temp->right = ln->hn;
					temp = temp->right;
					temp->parent = par;
				}
				else {
					par = temp;
					temp->left = ln->hn;
					temp = temp->left;
					temp->parent = par;
				}
			}
			else{
				if (bitString[i]) {
					par = temp;
					temp = temp->right;
				}
				else {
					par = temp;
					temp = temp->left;
				}
			}
		}
		free(bitString);
		bubbleUp(ln->hn);
		ln = ln->next;
	}
	freeHeapList(list);
}

int getInput(FILE *fp, int NumOfEntries1, int NumOfEntries2, int bucketSize, int totalRecs, Cost *costs, hashBucket **caller_table, hashBucket **callee_table, MaxHeap *heap){
	char buffer[200];
	char temp[200];
	int bye = 0;
	// int ops =0;

	while(fgets(buffer,sizeof(buffer),fp) != NULL){
		char* token;
        strcpy(temp, buffer);                  //in order to print the command given
		char* newline = strchr(buffer, '\n' ); //getting rid of newline character
		*newline = 0;
		if (temp[0] == ' ' || temp[0] == '\n' || temp[0] == '#') {
			continue;
		}
        if((token = strtok(buffer," ")) == NULL){
            fprintf(stderr,"Not an acceptable command for the program! Please re-run the program correctly!\n");
            return -1;
        }
		// printf("%d\n", ops++);
		if (strcmp(token, "insert") == 0) {
			char *cid, *orig, *dest, *date, *init;
			int dur, type, tar, fault;

			token = strtok(NULL, ";");
			cid = token;
			token = strtok(NULL, ";");
			orig = token;
			token = strtok(NULL, ";");
			dest = token;
			token = strtok(NULL, ";");
			date = token;
			token = strtok(NULL, ";");
			init = token;
			token = strtok(NULL, ";");
			if (!isNumber(strlen(token), token)) {
				fprintf(stderr,"Duration must be an integer value. Please re-enter this record.\n");
                continue;
			}
			dur = atoi(token);
			token = strtok(NULL, ";");
			if (!isNumber(strlen(token), token)) {
				fprintf(stderr,"Type must be an integer value. Please re-enter this record.\n");
                continue;
			}
			type = atoi(token);
			token = strtok(NULL, ";");
			if (!isNumber(strlen(token), token)) {
				fprintf(stderr,"Tariff must be an integer value. Please re-enter this record.\n");
                continue;
			}
			tar = atoi(token);
			token = strtok(NULL, ";");
			if (!isNumber(strlen(token), token)) {
				fprintf(stderr,"Fault Condition must be an integer value. Please re-enter this record.\n");
                continue;
			}
			fault = atoi(token);
			CDR *cdr = NULL;
			if ((cdr = createCDR(cdr, cid, orig, dest, date, init, dur, type, tar, fault)) == NULL) {
				fprintf(stderr, "IError\n");
				return -1;
			}
			if (insertHash(caller_table, cdr, cdr->originator_number, NumOfEntries1, totalRecs) < 0) {
				fprintf(stderr, "IError\n");
				return -1;
			}
			if (insertHash(callee_table, cdr, cdr->destination_number, NumOfEntries2, totalRecs) < 0) {
				fprintf(stderr, "IError\n");
				return -1;
			}
			if (heap->nodes == 0) {
				heap->nodes++;
				if ((heap->root = createHeapNode(heap, heap->root, cdr, costs)) == NULL) {
					fprintf(stderr, "IError\n");
					return -1;
				}
			}
			else {
				if (insertIntoHeap(heap, cdr, costs) < 0) {
					fprintf(stderr, "IError\n");
					return -1;
				}
			}
		}
		else if (strcmp(token, "delete") == 0) {
			char *caller, *id;
			token = strtok(NULL, " ");
			id = token;
			token = strtok(NULL, " ");
			caller = token;
			deleteHash(caller_table, caller, id, NumOfEntries1, totalRecs);
		}
		else if (strcmp(token, "find") == 0) {
			char *dateFrom, *dateTo, *timeFrom, *timeTo, *caller;
			dateFrom = dateTo = timeFrom = timeTo = NULL;
			token = strtok(NULL, " ");
			caller = token;
			if ((token = strtok(NULL, " ")) != NULL) {
				if (strlen(token) == 5 ) {
					timeFrom = token;
					token = strtok(NULL, " ");
					if (token == NULL) {
						fprintf(stderr, "Error ! expected more arguments in find ! Please re-enter this operation!\n");
						continue;
					}
					if (strlen(token) == 5) {
						timeTo = token;
					}
					else {
						dateFrom = token;
						token = strtok(NULL, " ");
						if (token == NULL) {
							fprintf(stderr, "Error ! expected more arguments in find ! Please re-enter this operation!\n");
							continue;
						}
						timeTo = token;
						token = strtok(NULL, " ");
						if (token == NULL) {
							fprintf(stderr, "Error ! expected more arguments in find ! Please re-enter this operation!\n");
							continue;
						}
						dateTo = token;
					}
				}
				else {
					dateFrom = token;
					token = strtok(NULL, " ");
					if (token == NULL) {
						fprintf(stderr, "Error ! expected more arguments in find ! Please re-enter this operation!\n");
						continue;
					}
					dateTo = token;
				}
			}
			printf("%s\n", temp);
			findHash(caller_table, caller, dateFrom, dateTo, timeFrom, timeTo, NumOfEntries1, totalRecs);
		}
		else if (strcmp(token, "lookup") == 0) {
			char *dateFrom, *dateTo, *timeFrom, *timeTo, *callee;
			dateFrom = dateTo = timeFrom = timeTo = NULL;
			token = strtok(NULL, " ");
			callee = token;
			if ((token = strtok(NULL, " ")) != NULL) {
				if (strlen(token) == 5 ) {
					timeFrom = token;
					token = strtok(NULL, " ");
					if (token == NULL) {
						fprintf(stderr, "Error ! expected more arguments in lookup ! Please re-enter this operation!\n");
						continue;
					}
					if (strlen(token) == 5) {
						timeTo = token;
					}
					else {
						dateFrom = token;
						token = strtok(NULL, " ");
						if (token == NULL) {
							fprintf(stderr, "Error ! expected more arguments in lookup ! Please re-enter this operation!\n");
							continue;
						}
						timeTo = token;
						token = strtok(NULL, " ");
						if (token == NULL) {
							fprintf(stderr, "Error ! expected more arguments in lookup ! Please re-enter this operation!\n");
							continue;
						}
						dateTo = token;
					}
				}
				else {
					dateFrom = token;
					token = strtok(NULL, " ");
					if (token == NULL) {
						fprintf(stderr, "Error ! expected more arguments in lookup ! Please re-enter this operation!\n");
						continue;
					}
					dateTo = token;
				}
			}
			printf("%s\n", temp);
			findHash(callee_table, callee, dateFrom, dateTo, timeFrom, timeTo, NumOfEntries1, totalRecs);
		}
		else if (strcmp(token, "indist") == 0) {
			char *caller1, *caller2;
			token = strtok(NULL, " ");
			caller1 = token;
			token = strtok(NULL, " ");
			caller2 = token;
			printf("%s\n", temp);
			if (indist(caller_table, callee_table, caller1, caller2, NumOfEntries1, NumOfEntries2, totalRecs) < 0) {
				printf("One of the two callers does not exist !\n");
			}
		}
		else if (strcmp(token, "topdest") == 0) {
			char *caller;
			token = strtok(NULL, " ");
			caller = token;
			printf("%s\n", temp);
			topDest(caller_table, caller, NumOfEntries1, totalRecs);
		}
		else if (strcmp(token, "top") == 0) {
			int percent;
			token = strtok(NULL, " ");
			percent = atoi(token);
			printf("%s\n", temp);
			topK(heap, percent);
		}
		else if (strcmp(token, "print") == 0) {
			token = strtok(NULL, " ");
			if (strcmp(token, "hashtable1") == 0) {
				printf("%s\n", temp);
				printHashTable(caller_table, NumOfEntries1, totalRecs, stdout);
			}
			else if (strcmp(token, "hashtable2") == 0) {
				printf("%s\n", temp);
				printHashTable(callee_table, NumOfEntries2, totalRecs, stdout);
			}
		}
		else if (strcmp(token, "dump") == 0) {
			char *dumpHash;
			FILE *dump_fp;
			token = strtok(NULL, " ");
			dumpHash = token;
			token = strtok(NULL, " ");
			if ((dump_fp = fopen(token, "w")) == 0) {
				fprintf(stderr,"Error ! Can't open dump file! \n");
				return -1;
			}
			if (strcmp(dumpHash, "hashtable1") == 0) {
				printf("%s\n", temp);
				printHashTable(caller_table, NumOfEntries1, totalRecs, dump_fp);
			}
			else if (strcmp(dumpHash, "hashtable2") == 0) {
				printf("%s\n", temp);
				printHashTable(callee_table, NumOfEntries2, totalRecs, dump_fp);
			}
			fclose(dump_fp);
		}
		else if (strcmp(token, "bye") == 0) {
			fprintf(stdout, "End of Current Input Method! Bye!\n");
			bye = 1;
			return 1;
		}
		else if (strcmp(token, "exit") == 0) {
			fprintf(stdout, "End of the program! Cya!\n");
			return 0;
		}
		else {
			fprintf(stderr,"Not an acceptable command for the program! Please re-run the program correctly!\n");
			return -1;
		}
	}
	return 0;
}
