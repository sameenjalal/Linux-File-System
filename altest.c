#include "arraylist.h"
#include "private_api.h"
#include <stdio.h>

void main(){
	int i;
	arraylist *list = arraylist_new(4);

	//printf("Created a new arraylist with capacity %d\n", arraylist_capacity(list));

	arraylist_add(0, list);
	arraylist_add(5, list);
	arraylist_add(1, list);
	arraylist_add(5, list);
	arraylist_add(2, list);
	arraylist_add(5, list);
	arraylist_add(3, list);
	arraylist_add(5, list);
	arraylist_add(4, list);
	arraylist_add(5, list);

	for (i = 0; i < arraylist_size(list); i++){
		printf("%d\n", arraylist_get(i, list));
	}
	//printf("Created a new arraylist with capacity %d\n", arraylist_capacity(list));

	arraylist_delete(0, list);

	for (i = 0; i < arraylist_size(list); i++){
		printf("%d\n", arraylist_get(i, list));
	}
	arraylist_destroy(list);
	printf("Size of inode is %d\nsize of block is %d\n size of superblock is %d\n", sizeof(inode), sizeof(int), sizeof(superblock));
}
