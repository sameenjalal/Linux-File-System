#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stdlib.h>

#define DATATYPE void *

typedef struct arraylist{
	DATATYPE *data;
	int size;
	int capacity;
} arraylist;

arraylist *arraylist_new(int capacity);
int arraylist_add(DATATYPE data, arraylist *list);
int arraylist_set(int index, DATATYPE data, arraylist *list);
int arraylist_delete(int index, arraylist *list);
DATATYPE arraylist_get(int index, arraylist *list);
int arraylist_size(arraylist *list);
void arraylist_destroy(arraylist *list);
int arraylist_capacity(arraylist *list);

#endif
