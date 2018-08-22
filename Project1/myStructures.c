#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "myStructures.h"

int* createBitString(int nodes, int *len){
	*len = floor((double)log2((double)nodes)) + 1;
	int *bitString;

	if ((bitString = (int *)malloc((*len)*sizeof(int))) == NULL) {
		fprintf(stderr, "Error allocating bitstring array\n");
		return NULL;
	}
	for (int i = 0; i < *len; i++) {
		bitString[i] = nodes & 1;
		nodes = nodes >> 1;
	}
	return bitString;
}

float determineCost(Cost *cost, CDR *cdr){
	int k;
	if (cdr->fault_condition < 200 && cdr->fault_condition > 299) {
		return 0;
	}
	if (cdr->type == 0) {
		k = cdr->type;
		return cost[k].cost;
	}
	if (cdr->type == 1) {
		k = cdr->type * cdr->tariff;
	}
	else if (cdr->type == 2) {
		k = cdr->type + cdr->tariff;
	}
	return cost[k].cost * cdr->duration;
}

/*----------------------------------------------------------------------------------------------------*/

CDR* createCDR(CDR *cdr, char *c_u_i, char *o_n, char *d_n, char *date, char *init, int dur, int t, int tar, int f_c){

	if( (cdr = (CDR*) malloc( sizeof(CDR)) )== NULL ){
	        fprintf(stderr,"Memory Error for the new record!\n");
	        return NULL;
	}

	memset(cdr->cdr_uniq_id, '\0', sizeof(cdr->cdr_uniq_id));
	memset(cdr->originator_number, '\0', sizeof(cdr->originator_number));
	memset(cdr->destination_number, '\0', sizeof(cdr->destination_number));
	memset(cdr->date, '\0', sizeof(cdr->date));
	memset(cdr->init_time, '\0', sizeof(cdr->init_time));

	memcpy(cdr->cdr_uniq_id, c_u_i, sizeof(cdr->cdr_uniq_id)-1);
	memcpy(cdr->originator_number, o_n, sizeof(cdr->originator_number)-1);
	memcpy(cdr->destination_number, d_n, sizeof(cdr->destination_number)-1);
	memcpy(cdr->date, date, sizeof(cdr->date)-1);
	memcpy(cdr->init_time, init, sizeof(cdr->init_time)-1);
	cdr->duration = dur;
	cdr->type = t;
	cdr->tariff = tar;
	cdr->fault_condition = f_c;
	return cdr;
}

void deleteCDR(CDR *cdr){
	free(cdr);
}

void printCDR(CDR *cdr, FILE *print_fp){
	fprintf(print_fp, "\t| %s | %s | %s | %s | %s | %d | %d | %d | %d | \n", \
		cdr->cdr_uniq_id, cdr->originator_number, cdr->destination_number, cdr->date, cdr->init_time,\
		 cdr->duration, cdr->type, cdr->tariff, cdr->fault_condition);
}

/*----------------------------------------------------------------------------------------------------*/

int HashFunct(char *value, int hSize){
	unsigned long hash = 5381;
	int c;

	while((c = *value++)){
		hash = ((hash << 5) + hash) + c;
	}
	return (hash % hSize);
}

cdrBucket* createCDRBucket(cdrBucket *cb, cdrBucket *pcb, int totalRecs){
	if ((cb = (cdrBucket *)malloc(sizeof(cdrBucket))) == NULL) {
		fprintf(stderr, "Memory Error for a new CDR bucket!\n");
		return NULL;
	}
	cb->next = NULL;
	if (pcb == NULL) {
		cb->prev = cb;
	}
	else {
		cb->prev = pcb;
	}
	cb->cdrs = 0;
	if ((cb->table = (CDR **)malloc(totalRecs*sizeof(CDR*))) == NULL) {
		fprintf(stderr, "Memory Error for a new CDR table!\n");
		return NULL;
	}
	for (int i = 0; i < totalRecs; i++) {
		cb->table[i] = NULL;
	}
	return cb;
}

void deleteCDRBucket(cdrBucket *cb, int totalRecs){
	cb->prev->next = cb->next;
	for (int i = 0; i < totalRecs; i++) {
		cb->table[i] = NULL;
	}
	free(cb->table);
	free(cb);
}

Record* createRecord(Record *r, int totalRecs){
	if ((r = (Record *)malloc(sizeof(Record))) == NULL) {
		fprintf(stderr, "Memory Error for a new Record!\n");
		return NULL;
	}
	r->number = NULL;
	r->CDRamount = 0;
	r->header = NULL;
	if ((r->header = createCDRBucket(r->header, r->header, totalRecs)) == NULL) {
		return NULL;
	}
	return r;
}

hashBucket* createHashBucket(hashBucket *hb, int totalRecs){
	if ((hb = (hashBucket *)malloc(sizeof(hashBucket))) == NULL) {
		fprintf(stderr, "Memory Error for a new bucket!\n");
		return NULL;
	}
	hb->items = 0;
	hb->next = NULL;
	if ((hb->table = (Record **)malloc(totalRecs*sizeof(Record*))) == NULL) {
		fprintf(stderr, "Memory Error for a new records table!\n");
		return NULL;
	}
	for (int i = 0; i < totalRecs; i++) {
		hb->table[i] = NULL;
		if ((hb->table[i] = createRecord(hb->table[i], totalRecs)) == NULL) {
			return NULL;
		}
	}
	return hb;
}

int initHashTable(hashBucket ***table, int NumOfEntries, int totalRecs){
	if( (*table = (hashBucket**) malloc(NumOfEntries*sizeof(hashBucket*))) == NULL){
        fprintf(stderr,"Memory Error for the caller/callee hash table!\n");
        return -1;
    }
	for (int i = 0; i < NumOfEntries; i++) {
		(*table)[i] = NULL;
		if (((*table)[i] = createHashBucket((*table)[i], totalRecs)) == NULL) {
			return -1;
		}
	}
}

int insertHash(hashBucket **table, CDR *cdr, char *number, int NumOfEntries, int totalRecs){
	int found = 0;
	hashBucket *temp;
	Record *aux;
	cdrBucket *iter;
	int hash = HashFunct(number, NumOfEntries);
	temp = table[hash];
	while (temp != NULL) {
		for (int i = 0; i < temp->items; i++) {
			aux = temp->table[i];
			if (strcmp(aux->number, number) == 0) {
				found = 1;
				break;
			}
		}
		if (found) { //yparxei idi to noumero kataxorhmeno sto hashtable
			iter = aux->header;
			while (iter != NULL) {
				for (int k = 0; k < totalRecs; k++) {
					if (iter->table[k] == NULL) {
						iter->table[k] = cdr;
						iter->cdrs++;
						aux->CDRamount++;
						return 0;
					}
				}
				if (iter->next == NULL) {
					if ((iter->next = createCDRBucket(iter->next, iter, totalRecs)) == NULL) {
						return -1;
					}
				}
				iter = iter->next;
			}
		}
		else{
			if (temp->items != totalRecs) { //an yparxei xoros sto bucket bale kanonika to noumero kai to cdr tou
				aux = temp->table[temp->items];
				aux->number = number;
				aux->header->table[aux->header->cdrs] = cdr;
				aux->header->cdrs++;
				aux->CDRamount++;
				temp->items++;
				return 0;
			}
			else {
				if (temp->next == NULL) {
					if ((temp->next = createHashBucket(temp->next, totalRecs)) == NULL) {
						return -1;
					}
				}
				temp = temp->next;
			}
		}
	}
}

Record* searchHash(hashBucket **table, char *number, int NumOfEntries, int totalRecs){
	hashBucket *temp;
	Record *aux;

	int hash = HashFunct(number, NumOfEntries);
	temp = table[hash];

	while (temp != NULL) {
		for (int i = 0; i < temp->items; i++) {
			aux = temp->table[i];
			if (strcmp(aux->number, number) == 0) {
				return aux;
			}
		}
		temp = temp->next;
	}
	return NULL;
}

int deleteHash(hashBucket **table, char *caller, char *cdr_id, int NumOfEntries, int totalRecs){
	Record *aux;
	cdrBucket *iter;
	if ((aux = searchHash(table, caller, NumOfEntries, totalRecs)) != NULL) {
		iter = aux->header;
		while (iter != NULL) {
			for (int z = 0; z < totalRecs; z++) {
				if (iter->table[z] == NULL) {
					continue;
				}
				if (strcmp(iter->table[z]->cdr_uniq_id, cdr_id) == 0) {
					iter->table[z] = NULL;
					iter->cdrs--;
					aux->CDRamount--;
					printf("Deleted cdr-id %s!\n", cdr_id);
					if (iter->cdrs == 0) {
						deleteCDRBucket(iter, totalRecs);
					}
					if (aux->CDRamount == 0) {
						aux->header = NULL;
					}
					return 0;
				}
			}
			iter = iter->next;
		}
		printf("DError! A CDR with ID : %s wasnt found on Caller : %s\n", cdr_id, caller);
	}
	else {
		printf("DError! Caller : %s doesnt exists!\n", caller);
	}
	return 0;
}

void printHashTable(hashBucket **table, int NumOfEntries, int totalRecs, FILE *print_fp){
	// printf("\n");
	fprintf(print_fp, "---------------------Printing HashTable---------------------\n");
	for (int i = 0; i < NumOfEntries; i++) {
		fprintf(print_fp,"\n");
		hashBucket *temp;
		Record *aux;
		cdrBucket *info;
		temp = table[i];
		fprintf(print_fp,"At %d Index of HashTable\n", i);
		int bucketNum = 1;
		while (temp != NULL) {
			// printf("\n");
			fprintf(print_fp, "In bucket %d with items %d \n", bucketNum, temp->items);
			fprintf(print_fp, "------------------------------------------------------------\n");
			for (int j = 0; j < temp->items; j++) {
				aux = temp->table[j];
				fprintf(print_fp, "Number %s has %d CDRs below : \n", aux->number, aux->CDRamount);
				info = aux->header;
				while (info != NULL) {
					for (int z = 0; z < totalRecs; z++) {
						if (info->table[z] == NULL) {
							continue;
						}
						printCDR(info->table[z], print_fp);
					}
					info = info->next;
				}
			}
			temp = temp->next;
			bucketNum++;
			fprintf(print_fp, "------------------------------------------------------------\n");
		}
	}
}

void freeHash(hashBucket **table, int NumOfEntries, int totalRecs, int mode){
	hashBucket *temp, *temp1;
	Record *aux;
	cdrBucket *info, *info1;
	for (int i = 0; i < NumOfEntries; i++) {
		temp1 = table[i];
		while ((temp = temp1) != NULL) {
			for (int j = 0; j < totalRecs; j++) {
				if((aux = temp->table[j]) == NULL) {
					continue;
				}
				info1 = aux->header;
				while ((info = info1) != NULL) {
					for (int z = 0; z < totalRecs; z++) {
						if (info->table[z] == NULL) {
							continue;
						}
						if (mode) {
							free(info->table[z]);
						}
						else {
							info->table[z] = NULL;
						}
					}
					info1 = info1->next;
					free(info->table);
					free(info);
				}
				free(aux);
			}
			temp1 = temp1->next;
			free(temp->table);
			free(temp);
		}
	}
	free(table);
}

/*----------------------------------------------------------------------------------------------------*/

int initMaxHeap(MaxHeap **heap){
	if( (*heap = (MaxHeap *) malloc(sizeof(MaxHeap))) == NULL){
        fprintf(stderr,"Memory Error for the MaxHeap!\n");
        return -1;
    }
	(*heap)->companysIncome = 0;
	(*heap)->nodes = 0;
	(*heap)->root = NULL;
	return 0;
}

HeapNode* createHeapNode(MaxHeap *heap, HeapNode *hn, CDR *cdr, Cost *costs){
	if( (hn = (HeapNode *) malloc(sizeof(HeapNode))) == NULL){
        fprintf(stderr,"Memory Error for the a new HeapNode!\n");
        return NULL;
    }
	hn->left = NULL;
	hn->right = NULL;
	hn->parent = NULL;
	memset(hn->number, '\0', sizeof(hn->number));

	strcpy(hn->number, cdr->originator_number);
	hn->sumOfMoney = determineCost(costs, cdr);
	heap->companysIncome += hn->sumOfMoney;
	return hn;
}

void addMoreToAHeapNode(MaxHeap *heap, HeapNode *hn, HeapNode **last, int *found, CDR *cdr, Cost *costs){
	if (hn == NULL) return;
	addMoreToAHeapNode(heap, hn->left, last, found, cdr, costs);
	if (strcmp(cdr->originator_number, hn->number) == 0) {
		float aux;
		aux = determineCost(costs, cdr);
		hn->sumOfMoney += aux;
		heap->companysIncome += aux;
		*found = 1;
		(*last) = hn; //epistrefetai meso orismatos autos o deikths oste meta o kombos pou to periexomeno tou
		return;		  //tha exei tropoipoihthei na anebei mesa sto heap
	}
	addMoreToAHeapNode(heap, hn->right, last, found, cdr, costs);
}

HeapNode* placeNodetoHeap(MaxHeap *heap, CDR *cdr, Cost *costs){
	int len;
	int *bitString = createBitString(heap->nodes, &len);
	HeapNode *temp = heap->root, *par;
	for (int i = 0; i < len-1; i++) {
		if (i == len-2) {
			if (bitString[i]) {
				par = temp;
				if ((temp->right = createHeapNode(heap, temp->right, cdr, costs)) == NULL) {
					return NULL;
				}
				temp = temp->right;
				temp->parent = par;
			}
			else {
				par = temp;
				if ((temp->left = createHeapNode(heap, temp->left, cdr, costs)) == NULL) {
					return NULL;
				}
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
	return temp;
}

int insertIntoHeap(MaxHeap *heap, CDR *cdr, Cost *costs){
	HeapNode *temp = heap->root;
	HeapNode *last = heap->root;
	int found = 0;
	addMoreToAHeapNode(heap, heap->root, &last, &found, cdr, costs);
	bubbleUp(last);

	if (found) {
		return 0;
	}

	heap->nodes++;
	// printf("Number %s does not exist on our heap! I am gonna insert it!\n", cdr->originator_number);
	if ((temp = placeNodetoHeap(heap, cdr, costs)) == NULL) {
		return -1;
	}
	bubbleUp(temp);
	return 0;
}

HeapNode* findLastNode(MaxHeap *heap){
	int len;
	int *bitString = createBitString(heap->nodes, &len);
	HeapNode *temp;
	temp = heap->root;

	for (int i = 0; i < len-1; i++) {
		if (bitString[i]) {
			temp = temp->right;
		}
		else {
			temp = temp->left;
		}
	}
	free(bitString);
	return temp;
}

void bubbleUp(HeapNode *hn){
	if (hn->parent == NULL) {
		return;
	}

	if (hn->parent->sumOfMoney < hn->sumOfMoney) {
		char num[15];
		float sum;
		memset(num, '\0', sizeof(num));

		strcpy(num, hn->number);
		sum = hn->sumOfMoney;
		strcpy(hn->number, hn->parent->number);
		hn->sumOfMoney = hn->parent->sumOfMoney;
		strcpy(hn->parent->number, num);
		hn->parent->sumOfMoney = sum;

		bubbleUp(hn->parent);
	}
}

void heapify(HeapNode *hn){
	char num[15];
	float sum;
	memset(num, '\0', sizeof(num));

	if (hn->left != NULL && hn->right != NULL) { //yparxoun kai ta dio paidia
		if (hn->sumOfMoney < hn->left->sumOfMoney && hn->sumOfMoney < hn->right->sumOfMoney) { //einai mikrotero kai ap ota dio paidia
			if (hn->left->sumOfMoney > hn->right->sumOfMoney) { //to aristero einai megalitero
				strcpy(num, hn->number);
				sum = hn->sumOfMoney;
				strcpy(hn->number, hn->left->number);
				hn->sumOfMoney = hn->left->sumOfMoney;
				strcpy(hn->left->number, num);
				hn->left->sumOfMoney = sum;

				heapify(hn->left);
			}
			else { //to dexi einai megalitero
				strcpy(num, hn->number);
				sum = hn->sumOfMoney;
				strcpy(hn->number, hn->right->number);
				hn->sumOfMoney = hn->right->sumOfMoney;
				strcpy(hn->right->number, num);
				hn->right->sumOfMoney = sum;

				heapify(hn->right);
			}
		}
		else if (hn->sumOfMoney < hn->left->sumOfMoney && hn->sumOfMoney > hn->right->sumOfMoney) { //einai mikrotero mono apo to aristero paidi
			strcpy(num, hn->number);
			sum = hn->sumOfMoney;
			strcpy(hn->number, hn->left->number);
			hn->sumOfMoney = hn->left->sumOfMoney;
			strcpy(hn->left->number, num);
			hn->left->sumOfMoney = sum;

			heapify(hn->left);
		}
		else if (hn->sumOfMoney > hn->left->sumOfMoney && hn->sumOfMoney < hn->right->sumOfMoney) { //einai mikrotero mono apo to deksi paidi
			strcpy(num, hn->number);
			sum = hn->sumOfMoney;
			strcpy(hn->number, hn->right->number);
			hn->sumOfMoney = hn->right->sumOfMoney;
			strcpy(hn->right->number, num);
			hn->right->sumOfMoney = sum;

			heapify(hn->right);
		}
	}
	else if (hn->left != NULL && hn->right == NULL) { //yparxei mono to aristero paidi
		if (hn->sumOfMoney < hn->left->sumOfMoney) { //einai mikrotero apo to aristero
			strcpy(num, hn->number);
			sum = hn->sumOfMoney;
			strcpy(hn->number, hn->left->number);
			hn->sumOfMoney = hn->left->sumOfMoney;
			strcpy(hn->left->number, num);
			hn->left->sumOfMoney = sum;

			heapify(hn->left);
		}
	}
}

void freeSubTree(HeapNode *hn){
	if (hn == NULL) {
		return;
	}
	if (hn->left == NULL && hn->right == NULL) {
		free(hn);
		return;
	}
	freeSubTree(hn->left);
	freeSubTree(hn->right);
	free(hn);
	return;
}

void printHeapNode(HeapNode *hn){
	printf("| Caller : %s spend %.2f money \n", hn->number, hn->sumOfMoney);
}

void printMaxHeap(HeapNode *hn){
	if (hn == NULL) {
		return;
	}
	printMaxHeap(hn->left);
	printHeapNode(hn);
	printMaxHeap(hn->right);
}

/*----------------------------------------------------------------------------------------------------*/

void createList(indistList **list){
	*list = (indistList*) malloc(sizeof(indistList));
	(*list)->count = 0;
	(*list)->header = NULL;
}

indistNode* createListNode(char *number){
	indistNode *node;

	node = (indistNode*)malloc(sizeof(indistNode));
	memset(node->number, '\0', sizeof(node->number));
	strcpy(node->number, number);
	node->next = NULL;
	return node;
}

void printList(indistList *list){
	indistNode *temp;

	temp = list->header;
	while (temp != NULL) {
		if (strlen(temp->number) != 0) {
			printf("%s\n", temp->number);
		}
		temp = temp->next;
	}
}

void freeList(indistList *list){
	indistNode *aux, *temp;

	aux = list->header;
	while ((temp = aux ) != NULL) {
		aux = aux->next;
		free(temp);
	}
	free(list);
}

/*----------------------------------------------------------------------------------------------------*/

void createHeapList(HeapList **list){
	(*list) = (HeapList *)malloc(sizeof(HeapList));
	(*list)->header = NULL;
}

HeapListNode* createHeapListNode(){
	HeapListNode *ln;

	ln = (HeapListNode *)malloc(sizeof(HeapListNode));
	ln->hn = NULL;
	ln->next = NULL;
	return ln;
}

void freeHeapList(HeapList *list){
	HeapListNode *temp, *aux;

	aux = list->header;
	while ((temp = aux) != NULL) {
		aux = aux->next;
		free(temp);
	}
	free(list);
}
