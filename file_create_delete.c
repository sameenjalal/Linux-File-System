// This file contains functions for creating and delting files

#include "api.h"
#include "private_api.h"

int file_create(char *path)
{
	int st = file_create_generic(path, 0);
	return st;
}

int file_delete(char *path){
	return file_delete_generic(path, 0);
}
