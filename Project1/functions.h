#include "myStructures.h"

int isNumber(int len, char *string);
int compareDates(char *dateFrom, char *dateTo, char *dateNow);
int count_occur(char **table, char *map, int amount, int start);
void countCountries(char **table, int amount);
int findHash(hashBucket **table, char *caller, char *dateFrom, char *dateTo, char *timeFrom, char *timeTo, int NumOfEntries, int totalRecs);
void contactedWith(hashBucket **table, indistList *list, char *caller, int NumOfEntries, int totalRecs, int mode);
indistList* sectionOfLists(indistList *list1, indistList *list2);
void diffCallers(hashBucket **table, indistList *list, int NumOfEntries, int totalRecs);
int indist(hashBucket **caller_table, hashBucket **callee_table, char *caller1, char *caller2, int NumOfEntries1, int NumOfEntries2, int totalRecs);
int topDest(hashBucket **table, char *caller, int NumOfEntries, int totalRecs);
void topK(MaxHeap *heap, int percent);
int getInput(FILE *fp, int NumOfEntries1, int NumOfEntries2, int bucketSize, int totalRecs, Cost *costs, hashBucket **caller_table, hashBucket **callee_table, MaxHeap *heap);
