#ifndef PRIVATEAPI_H
#define PRIVATEAPI_H

#include "arraylist.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BLOCK_SIZE 512
#define INODE_SIZE 64
#define DIRENTRY_SIZE 16
#define STATUS_OPEN 1
#define STATUS_CLOSED 0
#define FILESYSTEM_NAME "ssjfs"
#define ROOT_INODE 0
#define NON_INDIRECT_MAX 5120
#define FIRST_INDIRECT_MAX 70656
#define SECOND_INDIRECT_MAX 8459264

/* ONE IN X BLOCKS WILL BE DEVOTED TO INODES*/
/* Higher # = fewer inodes*/
#define INODE_OVERHEAD 20

// file descriptor pointing to the "disk file"

typedef struct file_info
{
	// path of file
	char filepath[100];

	// inode of the file
	int inumber;

	// Current byte offset in the file
	int cursor;
} file_info;

typedef struct superblock
{
	// The superblock includes:

	// What type of filesystem it is
	char fs_type[24];

	// size of disk in blocks
	int block_count;

	// count of current number of files
	int current_files;

	// count of max number of files
	int max_files;

	// count of blocks allocated
	int allocated_blocks;

	// pointer to the ilist
	int first_free_inode;

	// pointer to the first free block
	int first_free_block;

	// And padding to make it 512 bytes/..
	char padding[BLOCK_SIZE - (24 + (sizeof(int) * 6))];
} superblock;

typedef struct free_block
{
	// A normal block includes:

	// pointer to next free block
	int next_free;
	int prev_free;

	// Free status
	char data[BLOCK_SIZE - (sizeof(int) * 2)];
} free_block;

typedef struct inode
{
	// pointer to next free inode
	int next_free;
	int prev_free;

	// inodes contain:
	// filesize
	int filesize;

	// flag that indicates if it's a directory
	// 0 for file
	// 1 for dir
	int directory_flag;

	// pointers to data blocks
	int block_pointers[10];
	int indirect_pointer;
	int double_indirect_pointer;

	// and padding to make it 64 bytes...
	/*
	char padding[INODE_SIZE - (sizeof(int) * 16)];
	*/
} inode;

typedef struct direntry{
	char name[12];
	int inumber;
} direntry;

int fs_disk;
arraylist *open_files;
char disk_path[100];

int myread(int fd, void *buf, int num_blocks); //DONE
int mywrite(int fd, const void *buf, int num_blocks); //DONE
int mylseek(int fd, int block_offset, int whence); //DONE
inode *get_inode(int inumber); //DONE
free_block *load_free_block(int blockno); // DONE
int write_inode(inode *node, int inumber); // DONE
superblock *get_superblock(); // DONE
char **tokenize_path(char *path); // DONE
void free_path_tokens(char **path); // DONE
int path_to_inumber(char *path); // TODO See if affected by zero entry thingy
int get_inode_block(int inumber); // DONE
int get_absolute_block(int inumber, int block_offset); // DONE
int get_absolute_block_from_node(inode *node, int block_offset); // DONE
direntry **get_dirents(inode *dir); // DONE
direntry *block_dirents(char *buf, int count); // DONE
void free_dirents(direntry **dirents); // DONE
int get_new_inode(); // DONE
int get_new_block(); // DONE
int add_block_to_inode(inode *node, int new_block); // DONE
int file_delete_generic(char *path, int flag); // DONE
int file_create_generic(char *path, int flag); // DONE
int isDirentZeroed(direntry *dp); // DONE
int release_inode(int inumber); // DONE
int release_block(int blocknumber); // DONE

int add_new_file(int inumber, char *name); // TODO DEP.
char *get_first_avail_block(inode *node); // TODO DEP.

#endif
