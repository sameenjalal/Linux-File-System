#include "arraylist.h"

arraylist *arraylist_new(int cap){
	arraylist *ret;

	ret = (arraylist *)malloc(sizeof(arraylist));
	ret->data = (DATATYPE *)malloc(sizeof(DATATYPE) * cap);
	ret->size = 0;
	ret->capacity = cap;

	return ret;
}

int arraylist_add(DATATYPE new_value, arraylist *list){
	
	DATATYPE *data_array;
	int size = list->size;
	int index;

	// If we're at capacity, transfer everything to a double capacity array
	if (size == list->capacity){
		data_array = (DATATYPE *)malloc(sizeof(DATATYPE) * (size * 2));
		for (index = 0; index < size; index++){
			data_array[index] = list->data[index];
		}
		free(list->data);

		list->data = data_array;
		list->capacity = size * 2;
	}
	else{
		data_array = list->data;
	}

	// insert the new value
	data_array[size] = new_value;
	list->size++;

	return size;
}

int arraylist_set(int index, DATATYPE data, arraylist *list){
	if (index >= list->size || index < 0){
		return 1;
	}
	
	list->data[index] = data;

	return 0;
}

int arraylist_delete(int index, arraylist *list){
	int i;

	if (index < 0 || index >= list->size){
		return 1;
	}

	for (i = index; i < list->size - 1; i++){
		list->data[i] = list->data[i + 1];
	}
	list->size--;

	return 0;
}

DATATYPE arraylist_get(int index, arraylist *list){
	DATATYPE ret;

	if (index >= list->size || index < 0){
		return NULL;
	}

	ret = list->data[index];

	return ret;
}

int arraylist_size(arraylist *list){
	return list->size;
}

int arraylist_capacity(arraylist *list){
	return list->capacity;
}

void arraylist_destroy(arraylist *list){
	free(list->data);
	free(list);
}
