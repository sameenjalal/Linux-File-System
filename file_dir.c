// This file contains functions for creating, deleting, and reading directories
#include "api.h"
#include "private_api.h"
#include <string.h>

// Creates the specified directory.
// Returns an error or SUCCESS.
int file_mkdir(char *path)
{
	return file_create_generic(path, 1);
}

// Removes the specified directory.
// Returns an error or SUCCESS.
int file_rmdir(char *path)
{
	return file_delete_generic(path, 1);
}

// Lists the given directory.
// Returns an array of character strings.
char **file_listdir(char *path)
{
	char **ret;
	direntry *dp = NULL;
	direntry **dirents;
	int inumber;
	inode *iptr;
	int dirent_count = 0, i;

	inumber = path_to_inumber(path);
	iptr = get_inode(inumber);
	dirents = get_dirents(iptr);
	
	for (i = 0; dirents[i] != NULL; i++){
		dp = dirents[dirent_count];
		if (!isDirentZeroed(dp)){
			dirent_count++;
		}
	}	

	ret = (char **)malloc(sizeof(char) * (1 + dirent_count));
	for (i = 0; i < dirent_count; i++){
		ret[i] = strdup(dirents[i]->name);
	}
	ret[dirent_count] = NULL;

	free(iptr);
	//free_dirents(dirents);

	return ret;
	/*
    char *dup = strdup(path);
    char *tokenized_str;
    int i;
    char *current_dir;
    myread(fs_disk, current_dir, 1);
    char *tmp = NULL;
    while ( tokenized_str = strtok(dup, '/') )
    {
        if ( tmp = dir_contains(current_dir, tokenized_str) )
            current_dir = tmp;
        else break;
    }
	*/
}

// Prints a directory listing to the screen.
void file_printdir(char *path)
{
	char **names;
	char *ptr;
	int i;

	names = file_listdir(path);
	for (i = 0; names[i] != NULL; i++){
		fprintf(stdout, "%s\n", names[i]);
	}

	//free_path_tokens(names);
}
