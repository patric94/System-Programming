typedef struct Cost {
	int type;
	int tariff;
	float cost;
}Cost;

typedef struct	CDR {
	char cdr_uniq_id[11];
	char originator_number[15];
	char destination_number[15];
	char init_time[6];
	char date[9];
	int duration;
	int type;
	int tariff;
	int fault_condition;
}CDR;

typedef struct cdrBucket {
	int cdrs;
	CDR **table;
	struct cdrBucket *next;
	struct cdrBucket *prev;
}cdrBucket;

typedef struct Record { //   Number/Bucket tuple.
	int CDRamount;
	char* number;
	struct cdrBucket *header;
}Record;

typedef struct hashBucket {
	int items;
	struct Record **table;
	struct hashBucket *next;
}hashBucket;

typedef struct HeapNode {
	float sumOfMoney;
	char number[15];
	struct HeapNode *parent;
	struct HeapNode *left;
	struct HeapNode *right;
}HeapNode;

typedef struct MaxHeap {
	float companysIncome;
	int nodes;
	struct HeapNode *root;
}MaxHeap;

typedef struct indistNode {
	char number[15];
	struct indistNode *next;
}indistNode;

typedef struct indistList {
	int count;
	struct indistNode *header;
	struct indistNode *last;
}indistList;

typedef struct	HeapListNode {
	struct HeapNode *hn;
	struct HeapListNode *next;
}HeapListNode;

typedef struct HeapList {
	struct HeapListNode *header;
	struct HeapListNode *bottom;
}HeapList;

/*
Gia thn ylopoish toy heap symbouleutika to parakato link
http://theoryofprogramming.com/2015/02/01/binary-heaps-and-heapsort-algorithm/
*/


int* createBitString(int nodes, int *len);
float determineCost(Cost *cost, CDR *cdr);
CDR* createCDR(CDR *cdr, char *c_u_i, char *o_n, char *d_n, char *date, char *init, int dur, int t, int tar, int f_c);
void deleteCDR(CDR *cdr);
void printCDR(CDR *cdr, FILE *print_fp);
int HashFunct(char *value, int hSize);
cdrBucket* createCDRBucket(cdrBucket *cb, cdrBucket *pcb, int totalRecs);
void deleteCDRBucket(cdrBucket *cb, int totalRecs);
Record* createRecord(Record *r, int totalRecs);
hashBucket* createHashBucket(hashBucket *hb, int totalRecs);
int initHashTable(hashBucket ***table, int NumOfEntries, int totalRecs);
int insertHash(hashBucket **table, CDR *cdr, char *number, int NumOfEntries, int totalRecs);
Record* searchHash(hashBucket **table, char *number, int NumOfEntries, int totalRecs);
int deleteHash(hashBucket **table, char *caller, char *cdr_id, int NumOfEntries, int totalRecs);
void printHashTable(hashBucket **table, int NumOfEntries, int totalRecs, FILE *print_fp);
void freeHash(hashBucket **table, int NumOfEntries, int totalRecs, int mode);
int initMaxHeap(MaxHeap **heap);
HeapNode* createHeapNode(MaxHeap *heap, HeapNode *hn, CDR *cdr, Cost *costs);
void addMoreToAHeapNode(MaxHeap *heap, HeapNode *hn, HeapNode **last, int *found, CDR *cdr, Cost *costs);
HeapNode* placeNodetoHeap(MaxHeap *heap, CDR *cdr, Cost *costs);
int insertIntoHeap(MaxHeap *heap, CDR *cdr, Cost *costs);
HeapNode* findLastNode(MaxHeap *heap);
void bubbleUp(HeapNode *hn);
void heapify(HeapNode *hn);
void freeSubTree(HeapNode *hn);
void printHeapNode(HeapNode *hn);
void printMaxHeap(HeapNode *hn);
void createList(indistList **list);
indistNode* createListNode(char *number);
void printList(indistList *list);
void freeList(indistList *list);
void createHeapList(HeapList **list);
HeapListNode* createHeapListNode();
void freeHeapList(HeapList *list);
