// This file contains functions for opening and closing files

#include "api.h"
#include "private_api.h"

int
file_open(char *path)
{
	// Check if file exists
	int file_num;
    int inumber = path_to_inumber(path);
	printf("path_to_inumber returned %d\n", inumber);
	if ( inumber <= 0 )
        return ERR_FILE_EXISTS;

	// Check if file or dir
	inode *in = get_inode(inumber);
	printf("get_inode returned %p\n", in);
	if ( in->directory_flag )
		return ERR_FILE_NOT_FOUND;

    // make file_info and add to open_files arraylist
	file_info *new_file = (file_info *) malloc(sizeof(file_info));
	strcpy(new_file->filepath, path);
	new_file->inumber = inumber;
	new_file->cursor = 0;

	file_num = arraylist_add(new_file,open_files);
	return file_num;
}

void
file_close(int file_number)
{
	// Delete file from arraylist "open_files"
	if ( !arraylist_delete(file_number, open_files) )
		return;
}
