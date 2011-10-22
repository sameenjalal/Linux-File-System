// This file contains abstractions for read(), write() and lseek()
// as well as all required backend functionailty for the functions
// presented to the outside world

#include "private_api.h"
#include "api.h"

// Read some number of blocks from the file described by fd
// Returns the number of blocks read
int myread(int fd, void *buf, int num_blocks){
//	printf("entering myread, %d, %p, %d\n", fd, buf, num_blocks);
	int num_blocks_read = 0;
	int num_bytes_read = 0;

	num_bytes_read = read(fd, buf, num_blocks * BLOCK_SIZE);
//	printf("%d bytes read from disk\n", num_bytes_read);
	num_blocks_read = num_bytes_read / BLOCK_SIZE;

//	printf("Exiting myread\n");
	/*
	close(fs_disk);
	fs_disk = open(disk_path, O_RDWR | O_NONBLOCK);*/
	return num_blocks_read;
}

// Writes the specified number of blocks to the file described by fd
// Returns the number of blocks written
int mywrite(int fd, const void *buf, int num_blocks){
	int num_blocks_writ = 0;
	int num_bytes_writ = 0;

	num_bytes_writ = write(fd, buf, num_blocks * BLOCK_SIZE);
	num_blocks_writ = num_bytes_writ / BLOCK_SIZE;

	return num_blocks_writ;
}

// Seeks to a specified block in the "disk"
// Returns the block number of the result
int mylseek(int fd, int block_offset, int whence){
	off_t byte_offset = lseek(fd, block_offset * BLOCK_SIZE, whence);
//	printf("seeking to %d - aka block %d\n", byte_offset, block_offset);
	return byte_offset / BLOCK_SIZE;
}

// Gets an inode from the "disk" based on the inumber
inode *get_inode(int inumber){
	superblock *sb;
	char block[BLOCK_SIZE];
	inode *ret;
	int block_number, block_offset;

	// First, get the superblock so we can find out where the inodes are and how many there are
	sb = get_superblock();
	if (inumber < 0 || inumber > sb->max_files){
		printf("inumber out of range\n");
		free(sb);
		return NULL;
	}
	free(sb);
	// Now we need to pull the block containing the inode off the disk
	block_number = 1 + (inumber / (BLOCK_SIZE / INODE_SIZE));
	mylseek(fs_disk, block_number, SEEK_SET);
	if (myread(fs_disk, block, 1) != 1){
		printf("myread didnt return 1\n");
		return NULL;
	}

	// Finally, extract the inode from the block
	block_offset = inumber % (BLOCK_SIZE / INODE_SIZE);
	ret = (inode *)malloc(sizeof(inode));
//	printf("Getting inode %d from block %d offset %d\n", inumber, block_number, block_offset);
	memcpy(ret, &block[block_offset * INODE_SIZE], INODE_SIZE);

	return ret;
}

// get a block and interpret it as a free block
free_block *load_free_block(int blockno){
	free_block *ret;
	superblock *sb;
	
	sb = get_superblock();
	if (blockno < 0 || blockno > sb->block_count){
		return NULL;
	}

	// Now we need to pull the block off the disk and return it
	ret = (free_block *)malloc(BLOCK_SIZE);
	mylseek(fs_disk, blockno, SEEK_SET);
	if (myread(fs_disk, ret, 1) != 1){
		return NULL;
	}

	free(sb);
	return ret;
}

// given the inode of a directory, return an array of the direntries contained within
direntry **get_dirents(inode *dir){
	// blocks in the directory file
	int dir_block_count, index, dirents_remaining, total_dirents, ret_index = 0, ret_base, i;
	int subarray_dirents, dirents_in_a_block;
	char buffer[BLOCK_SIZE];
	int total_index = 0, abs_block;
	direntry *subarray;
	direntry **ret;
	// get number of blocks in this directory, and number of dirents per block
	dir_block_count = (dir->filesize / BLOCK_SIZE) + 1;
	dirents_in_a_block = BLOCK_SIZE / DIRENTRY_SIZE;

	// get total number of dirents in this directory
	total_dirents = dir->filesize / DIRENTRY_SIZE;
	dirents_remaining = total_dirents;

	ret = (direntry **)malloc(sizeof(direntry *) * (1 + total_dirents));

	for (index = 0; index < dir_block_count; index++){
		abs_block = get_absolute_block_from_node(dir, index);
		mylseek(fs_disk, abs_block, SEEK_SET);
		myread(fs_disk, buffer, 1);
		for (i = 0; i * DIRENTRY_SIZE < BLOCK_SIZE && dirents_remaining > 0; i++){
			ret[total_index] = &buffer[i * DIRENTRY_SIZE];
			total_index++;
			dirents_remaining--;
		}
/*
		subarray = block_dirents(buffer, subarray_dirents);
		ret_base = total_dirents - dirents_remaining;
		for (i = 0; ret_index < ret_base; ret_index++){
			ret[ret_index] = (direntry *)malloc(DIRENTRY_SIZE);
			memcpy(ret[ret_index], &subarray[i], DIRENTRY_SIZE);
			printf("%s\n", subarray[i].name);
		}
		free(subarray);*/
	}
	ret[total_dirents] = NULL;

	return ret;
}

// cleans up the product of get_dirents
void free_dirents(direntry **dirents){
	direntry *ptr;
	int i;	
	if (dirents == NULL)
		return;

	for (i = 0, ptr = dirents[0]; dirents[i] != NULL; i++){
		free(dirents[i]);
	}
	free(dirents);
}

// given a block of data, get the COUNT directory entries contained within
direntry *block_dirents(char *buf, int count){
	int i;
	direntry *ret;

	// get shit into ret
	ret = (direntry *)malloc(sizeof(direntry) * count);
	for (i = 0; i < count; i++){
		printf("%s\n", &buf[sizeof(direntry) * i]);
		memcpy(&ret[i], &buf[sizeof(direntry) * i], sizeof(direntry));
	}

	return ret;
}

int isDirentZeroed(direntry *dp){
	int i;
	char *c;
	c = (char *)dp;

	for (i = 0; i < DIRENTRY_SIZE; i++){
		if ((unsigned char)(c[i]) != (unsigned char)0){
			return 0;
		}
	}

	return 1;
}

// get the superblock!
superblock *get_superblock(){
	superblock *ret;

	ret = (superblock *)malloc(BLOCK_SIZE);
//	printf("seeking to superblock, fs_disk is %d\n", fs_disk);
	mylseek(fs_disk, 0, SEEK_SET);
//	printf("reading superblock, fs_disk is %d\n", fs_disk);
	if (myread(fs_disk, ret, 1) != 1){
		return NULL;
	}

//	printf("Exiting get_sb\n");
	return ret;
}

// Writes a memory image of an inode to disk at the specified inumber
int write_inode(inode *node, int inumber){
	int block_pointer, inode_offset;
	char *buffer = malloc(BLOCK_SIZE);
//	char buffer[BLOCK_SIZE];

	// figure out where this inode is
	block_pointer = 1 + (inumber / (BLOCK_SIZE / INODE_SIZE));
	inode_offset = inumber % (BLOCK_SIZE / INODE_SIZE);
	
	//printf("Writing %d\n%d\n%d\nto block %d, offset %d\n", inumber, node->prev_free, node->next_free, block_pointer, inode_offset);
	// grab the block it is contained in
	mylseek(fs_disk, block_pointer, SEEK_SET);
	myread(fs_disk, buffer, 1);

	// write the new inode into the buffer
	memcpy(&buffer[inode_offset * INODE_SIZE], node, INODE_SIZE);
//	printf("buffer is %p, buffer + offset is %p\n", buffer, &buffer[inode_offset * INODE_SIZE]);

	// write the block back to disk
	mylseek(fs_disk, block_pointer, SEEK_SET);
	int st = mywrite(fs_disk, buffer, 1);	
	if (st != 1)
		printf("Write failed\n");
	free(buffer);
	return st;
}

// breaks a path into tokens (not including root directory)
// eg /usr/bin/vim will be broken into "usr", "bin", "vim"
char **tokenize_path(char *path){
	int length, i, tokens = 0;
	char *duplicate = strdup(path);
	char **ret;

	printf("Tokenizing path: %s\n", path);
	length = strlen(duplicate);
	if (duplicate[0] != '/')
		return NULL;

	// find the number of tokens
	for (i = 0; i < length; i++){
		if (duplicate[i] == '/')
			tokens++;
	}
	if (duplicate[length] == '/')
		tokens--;

	if (length == 1){
		tokens = 0;
	}

	// add each token to the list
	ret = (char **)malloc(sizeof(char *) * (tokens + 1));
	if (tokens > 0){
		ret[0] = strdup(strtok(duplicate, "/"));
	}
	for (i = 1; i < tokens; i++){
		ret[i] = strdup(strtok(NULL, "/"));
	}
	ret[tokens] = NULL;
	free(duplicate);

	return ret;
}

// cleans up the output of tokenize_path
void free_path_tokens(char **path){
	int i;
	for(i = 0; path[i] != NULL; i++){
		free(path[i]);
	}
	free(path);
}

// returns the inumber of the file or directory specified by path
int
path_to_inumber(char *path)
{
	if (!path){
		return ERR_FILE_NOT_FOUND;
	}
	if (strlen(path) == 0 || strcmp(path, "/") == 0){
		return 0;
	}
    // Tokenize path
//	printf("path %p%s\n",path, path);
    char **path_tok = tokenize_path(path);

	printf("ASKLDLAKSJD\n");
    // Get root inode and the current directory
    inode *root_inode = get_inode(0);
    direntry **current_dir = get_dirents(root_inode);
    int i, j = 0;
	for ( j = 0 ; path_tok[j] != NULL ; j++ )
	{
		for ( i = 0 ; current_dir[i] != NULL ; i++ )
		{
			if ( (strcmp(current_dir[i]->name, path_tok[j]) == 0) && (path_tok[j + 1]) == NULL ){
				printf("%d\n", current_dir[i]->inumber);
				return current_dir[i]->inumber;
			}

			if ( strcmp(current_dir[i]->name, path_tok[j]) == 0 )
			{
				printf("%s was equal to %s \n%s\n", path_tok[j], current_dir[i]->name, path_tok[j+1]);
				inode *a = get_inode(current_dir[i]->inumber);
				//free_dirents(current_dir);
				current_dir = get_dirents(a);
				//	free(a);
				break;
			}
		}
	}
	//free_dirents(current_dir);
	//free_path_tokens(path_tok);
	//free(root_inode);	
    return(ERR_INVALID_PATH);
}

// returns the number of a new block ripped from the free block pool
// If you don't do anything with this block it DIES
int get_new_block(){
	superblock *sb;
	int block_pointer; 
	free_block *this_block, *prev_block, *next_block;

	sb = get_superblock();
	block_pointer = sb->first_free_block;

	// load to memory
	this_block = load_free_block(block_pointer);
	prev_block = load_free_block(this_block->prev_free);
	next_block = load_free_block(this_block->next_free);

	printf("new block: %d, prev: %d, next: %d\n", block_pointer, this_block->prev_free
		, this_block->next_free);

	// point the stuffs at each other!
	prev_block->next_free = this_block->next_free;
	next_block->prev_free = this_block->prev_free;
	sb->first_free_block = this_block->next_free;

	// update superblock
	sb->allocated_blocks++;

	// write everything back to disk
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);

	mylseek(fs_disk, block_pointer, SEEK_SET);
	mywrite(fs_disk, this_block, 1);

	mylseek(fs_disk, this_block->next_free, SEEK_SET);
	mywrite(fs_disk, next_block, 1);

	mylseek(fs_disk, this_block->prev_free, SEEK_SET);
	mywrite(fs_disk, prev_block, 1);

	// cleanup
	free(sb);
	free(this_block);
	free(next_block);	
	free(prev_block);

//	printf("get_new_block returning %d\n", block_pointer);
	return block_pointer;
}

// rips a new inode from the pool of free inodes and fixes the list
// returns the inumber of this new inode.  USE IT OR IT DIES
int get_new_inode(){
//	printf("Entering get_new_inode\n");
	int ret, pnode, nnode;
	superblock *sb;
	inode *iptr, *nptr, *pptr;

//	printf("Getting super block\n");
	sb = get_superblock();
//	printf("Done getting super block\n");
	ret = sb->first_free_inode;

	// load this inode to memory so we can read its free neighbors
	// and go update their pointers
	//printf("Getting free inode from inumber %d\n", ret);
	iptr = get_inode(ret);
	pnode = iptr->prev_free;
	nnode = iptr->next_free;
//	printf("Getting previous and next pointers for %d: p %d n %d\n", ret, pnode, nnode);
	//printf("Done getting free inode from inumber %d\n", ret);
	iptr->filesize = 0;	
	
	// load the other two into memory, and update them
	pptr = get_inode(pnode);
	nptr = get_inode(nnode);

//	printf("pptr is %p, nptr is %p\n", pptr, nptr);
	pptr->next_free = nnode;
	nptr->prev_free = pnode;

//	printf("setting %d's next pointer to %d\n", pnode, nnode);
//	printf("setting %d's previous pointer to %d\n", nnode, pnode);

	// update superblock
	sb->current_files++;
	sb->first_free_inode = nnode;

	// write everything back to disk
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);
	write_inode(nptr, nnode);
	write_inode(pptr, pnode);
	//write_inode(iptr, ret);

	// cleanup
	free(iptr);
	free(nptr);
	free(pptr);
	free(sb);
//	printf("Exiting get new inode\n");
	return ret;
}

// inserts the block back into the free block list between the first and second
int release_block(int blocknumber){
	free_block *new_image, *first, *second;
	int first_bn, second_bn;
	superblock *sb;

	sb = get_superblock();
	new_image = (free_block *)malloc(BLOCK_SIZE);

	// special case, disk full
	if (sb->allocated_blocks == sb->block_count){
		sb->first_free_block = blocknumber;
		new_image->next_free = blocknumber;
		new_image->prev_free = blocknumber;
	}
	else{
		// get first and second
		first = load_free_block(sb->first_free_block);
		second = load_free_block(first->next_free);
		
		// set all the shit
		first->next_free = blocknumber;
		second->prev_free = blocknumber;
		new_image->prev_free = first_bn;
		new_image->next_free = second_bn;
		
		//write 1st and 2nd back
		mylseek(fs_disk, first_bn, SEEK_SET);
		mywrite(fs_disk, first, 1);
		mylseek(fs_disk, second_bn, SEEK_SET);
		mywrite(fs_disk, second, 1);
	
		free(first);
		free(second);
	}
	// write new back
	mylseek(fs_disk, blocknumber, SEEK_SET);
	mywrite(fs_disk, new_image, 1);

	// update sb and writeback
	sb->allocated_blocks--;
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);

	free(sb);
	free(new_image);
	return 0;
}

int release_inode(int inumber){
	inode *iptr, *first, *second;
	int first_n, second_n;
	int blocks_in_file;
	superblock *sb;
	int block_ptr, i;

	iptr = (inode *)malloc(sizeof(inode));
	sb = get_superblock();

	// release all blocks
	blocks_in_file = 1 + iptr->filesize / BLOCK_SIZE;
	if (iptr->filesize == 0)
		blocks_in_file = 0;
	for (i = 0; i < blocks_in_file; i++){
		block_ptr = get_absolute_block_from_node(iptr, i);
		release_block(block_ptr);
	}

	// special case max files
	if (sb->current_files == sb->max_files){
		sb->first_free_inode = inumber;
		iptr->next_free = inumber;
		iptr->prev_free = inumber;
	}
	else{
		// get 1st and 2nd inodes
		first_n = sb->first_free_inode;
		first = get_inode(first_n);

		second_n = first->next_free;
		second = get_inode(second_n);

		// set pointers
		first->next_free = inumber;
		second->prev_free = inumber;
		iptr->next_free = second_n;
		iptr->prev_free = first_n;

		// write 1st and 2nd back
		write_inode(first, first_n);
		write_inode(second, second_n);
	}

	// write new back
	write_inode(iptr, inumber);
	
	// update sb and write it back
	sb->current_files--;
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);

	free(iptr);
	free(first);
	free(second);
	free(sb);

	return 0;
}

// Wrapper for get_absolute_block_from_node for functions who dont feel
// like getting the inode themselves
int get_absolute_block(int inumber, int block_offset){
	int ret;
	inode *node = get_inode(inumber);
	ret = get_absolute_block_from_node(node, block_offset);
	free(node);
	return ret;
}

// returns the block number of the nth block (zero based) in a file
// e.g. "i want the 342nd block in this file please!" ok here you go
int get_absolute_block_from_node(inode *node, int block_offset){
	int used_blocks, ptr1, ptr2;
	int ret;
	char buffer[BLOCK_SIZE];

	used_blocks = node->filesize / BLOCK_SIZE;
	if (node->filesize % BLOCK_SIZE != 0){
		used_blocks++;
	}	

	if (block_offset > used_blocks - 1){
		return -1;
	}
	

	// no indirect
	if (block_offset < 10){
		ret = node->block_pointers[block_offset];
	}
	// single indirect
	else if (block_offset < 138){
		block_offset -= 10;
		ptr1 = block_offset;
		mylseek(fs_disk, node->indirect_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&ret, &buffer[ptr1], sizeof(int));
	}
	// double indirect
	else{
		block_offset -= 138;
		ptr1 = block_offset / 128;
		ptr2 = block_offset % 128;
		mylseek(fs_disk, node->double_indirect_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&ret, &buffer[ptr1], sizeof(int));

		mylseek(fs_disk, ret, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&ret, &buffer[ptr2], sizeof(int));
	}

//	printf("Finding %dth block in file: %d\n", block_offset, ret);
	return ret;
}

// have to write the inode back after
int add_block_to_inode(inode *node, int new_block){
	int block_offset, latest_block_in_file;
	int ptr1 = 12901, ptr2 = 19281;
	int second_level;
	char buffer[BLOCK_SIZE];

	// find latest block we actually have...
	latest_block_in_file = node->filesize / BLOCK_SIZE;
	if (node->filesize % BLOCK_SIZE == 0)
		latest_block_in_file--;

	block_offset = latest_block_in_file + 1;
	
	if (block_offset > MAX_FILESIZE / BLOCK_SIZE){
		return ERR_PAST_END;
	}
	// no indirect
	if (block_offset < 10){
		node->block_pointers[block_offset] = new_block;
	}
	// single indirect
	else if (block_offset < 138){
		// if this is the first single indirect block, have to add a single indirect ptrblk
		if (block_offset == 10){
			node->indirect_pointer = get_new_block();
		}
		block_offset -= 10;
		ptr1 = block_offset;
		mylseek(fs_disk, node->indirect_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&buffer[ptr1], &new_block, sizeof(int));
	}
	// double indirect
	else{
		block_offset -= 138;
		// if it's the 138th, add the first level indirection block
		if (block_offset == 0){
			node->double_indirect_pointer = get_new_block();
		}
		ptr1 = block_offset / 128;
		ptr2 = block_offset % 128;

		// every 128th block, make a new second level block, add its ptr to first level
		if (ptr2 == 0){
			second_level = get_new_block();
			mylseek(fs_disk, node->double_indirect_pointer, SEEK_SET);
			myread(fs_disk, buffer, 1);
			memcpy(&buffer[ptr1], &second_level, sizeof(int));
			mywrite(fs_disk, buffer, 1);
		}
		mylseek(fs_disk, node->double_indirect_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&second_level, &buffer[ptr1], sizeof(int));

		mylseek(fs_disk, second_level, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&buffer[ptr2], &new_block, sizeof(int));
		mywrite(fs_disk, buffer, 1);
	}

	return 0;
}

int get_inode_block(int inumber){
	return 1 + inumber / (BLOCK_SIZE / INODE_SIZE);
}

// generic file create for mkdir and filecreate
int file_create_generic(char *path, int flag)
{
	superblock *sb;
	int inumber, i, j, path_length, index_last_slash = 0, new_inumber;
	int block_pointer, block_in_file;
	int total_blocks, bytes_remaining, entry_pointer, bytes_in_this_block;
	int zero_entry_byte = -1, zero_entry_block = -1;
	inode *iptr, *new_iptr;
	char new_filename[12];
	char candidate[16];
	char *partial_path, *duplicate_path;
	char buffer[BLOCK_SIZE];

	sb = get_superblock();
//	printf("sb = %p curfiles = %d, max_files = %d\n", sb, sb->current_files, sb->max_files);

	if (sb->current_files == sb->max_files){
		return ERR_MAX_FILES;
	}
	if (sb->allocated_blocks == sb->block_count){
		return ERR_DISK_FULL;
	}
	// Remove the last token from the given path and keep it aside
	// That will be the new file name. 
	path_length = strlen(path);
	for (i = 1; i < path_length; i++){
		if (path[i] == '/'){
			index_last_slash = i;
		}
	}
	duplicate_path = strdup(path);
	duplicate_path[index_last_slash] = '\0';
	
	// There's no name for the new directory so lets pretend they tried to create the
	// one before the last slash, i.e.: /usr/BIN/
	if (index_last_slash == path_length - 1){
		return ERR_FILE_EXISTS;
	}

	memcpy(new_filename, &duplicate_path[index_last_slash + 1], 12);
	partial_path = duplicate_path;
//	printf("Calling path_to_inumber with %s\nNew filename is %s\n", partial_path, new_filename);
	// ok now get the inode of the partial path
	inumber = path_to_inumber(partial_path);
//	printf("inumber of dir is %d\n", inumber);
	// got an error back from path_to_inumber
	if (inumber < 0){
		free(duplicate_path);
		return ERR_INVALID_PATH;	
	}
	
	// load the inode
	iptr = get_inode(inumber);
	
	// get new inode as well
	new_inumber = get_new_inode();
//	printf("Got a new inode: %d\n", new_inumber); 

	// initialize new inode
	new_iptr = get_inode(new_inumber);
	new_iptr->filesize = 0;
	new_iptr->directory_flag = flag;

	// write back new inode
	write_inode(new_iptr, new_inumber);
	
	// See if there are any convenient zero entries to put it in
	total_blocks = iptr->filesize / BLOCK_SIZE;
	bytes_remaining = iptr->filesize;
	for (i = 0; i < total_blocks; i++){
		// load current block to the buffer
		block_pointer = get_absolute_block_from_node(iptr, i);
		mylseek(fs_disk, block_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);

		bytes_in_this_block = bytes_remaining % BLOCK_SIZE;
		bytes_remaining -= bytes_in_this_block;

		// iterate through entries in block
		for (entry_pointer = 0; entry_pointer < bytes_in_this_block; entry_pointer++){
			memcpy(&candidate, &buffer[entry_pointer], DIRENTRY_SIZE);
			for (j = 0; j < DIRENTRY_SIZE; j++){
				if (candidate[j] == '0'){
					zero_entry_byte = entry_pointer * DIRENTRY_SIZE;
					zero_entry_block = i;
					break;
				}
			}
		}
	}
	free(sb);
	sb = get_superblock();
		
	// found a zero thingy, use it!
	if (zero_entry_byte >= 0 && zero_entry_block >= 0){
		printf("Shouldnt be here\n");
		mylseek(fs_disk, zero_entry_block, SEEK_SET);
		myread(fs_disk, buffer, 1);
		memcpy(&buffer[zero_entry_byte], new_filename, 12);
		memcpy(&buffer[zero_entry_byte + 12], &new_inumber, 4);
		mywrite(fs_disk, buffer, SEEK_SET);
	}
	else{
		// find latest data block in inode
		block_in_file = iptr->filesize / BLOCK_SIZE;
		block_pointer = get_absolute_block_from_node(iptr, block_in_file);
		
		// see if we need to add a new block to the inode
		if (iptr->filesize % BLOCK_SIZE == 0){
			if (sb->block_count - sb->allocated_blocks == 1){
				return ERR_DISK_FULL;
			}
			block_pointer = get_new_block();
			add_block_to_inode(iptr, block_pointer);
			write_inode(iptr, inumber);
//			printf("Added block %d to inode\n", block_pointer);
		}
	
		// get inode's data block
		mylseek(fs_disk, block_pointer, SEEK_SET);
		myread(fs_disk, buffer, 1);
	
//		printf("name being added %s\n",new_filename);
		// update this inode's data block
		memcpy(&buffer[iptr->filesize % BLOCK_SIZE], new_filename, 12);
		memcpy(&buffer[iptr->filesize % BLOCK_SIZE + 12], &new_inumber, 4);
	
		// write it back
		mylseek(fs_disk, block_pointer, SEEK_SET);
		mywrite(fs_disk, buffer, 1);
//		printf("Updated block %d\n", block_pointer);
	}
	iptr->filesize += 16;
	// write back this directory's inode
	write_inode(iptr, inumber);

	free(duplicate_path);
	free(sb);
	free(iptr);
	free(new_iptr);
	return 0;
}

// generic file delete for rmdir and delete file
int file_delete_generic(char *path, int flag)
{
	int inumber, parent_inumber, block_pointer;
	inode *parent, *iptr;
	direntry **dirents;	
	direntry *dp;
	char buffer[BLOCK_SIZE];
	char *partial_path;
	int path_length;
	int i, index_last_slash;
	int dirents_in_parent, block_offset, byte_offset;
	// find if the directory exists
	
	inumber = path_to_inumber(path);
	if (inumber < 0){
		return ERR_FILE_NOT_FOUND;
	}

	iptr = get_inode(inumber);
	
	// check if it's the right type of file (dir or file)
	if (iptr->directory_flag != flag){
		return ERR_FILE_NOT_FOUND;
	}

	// Check if this directory is empty..
	if (iptr->filesize != 0 && iptr->directory_flag == 1){
		free(iptr);
		return ERR_FILE_EXISTS;
	}
	free(iptr);

	// Remove the last token from the given path and keep it aside
	// That will be the new file name. 
	path_length = strlen(path);
	for (i = 0; i < path_length; i++){
		if (path[i] == '/'){
			index_last_slash = i;
		}
	}
	partial_path = strdup(path);
	partial_path[index_last_slash] = '\0';

	parent_inumber = path_to_inumber(partial_path);
	free(partial_path);
	parent = get_inode(parent_inumber);

	// find the pointer to this directory in the parent and set it to zeros
	dirents = get_dirents(parent);
	dirents_in_parent = parent->filesize / DIRENTRY_SIZE;
	for (i = 0; i < dirents_in_parent; i++){
		dp = dirents[i];
		if (dp->inumber == inumber){
			block_offset = i / (BLOCK_SIZE / DIRENTRY_SIZE);
			byte_offset = DIRENTRY_SIZE * (i % (BLOCK_SIZE / DIRENTRY_SIZE));
			break;
		}
	}
	if (i == dirents_in_parent){
		return ERR_FILE_NOT_FOUND;
	}
	block_pointer = get_absolute_block_from_node(parent, block_offset);
	free(parent);

	// seek to the pointerblock
	mylseek(fs_disk, block_pointer, SEEK_SET);
	myread(fs_disk, buffer, 1);
	// erase the entry
	memset(&buffer[byte_offset], 0, 12);
	// write block back
	mywrite(fs_disk, buffer, 1);	

	// delete the inode for this directory
	release_inode(inumber);

	return 0;

}

/*
// adds an empty file called NAME to the directory specified by inumber
// mode specifies whether the new thing is a file or dir
int add_new_file(int inumber, char *name, char mode){
	inode *node;
	int num_used_blocks, first_avail_block, 
		
	node = get_inode(inumber);
	// if this isn't a directory
	if (node->directory != 1){
		return ERR_INVALID_PATH;
	}

	// get the latest data block
	first_avail_block = get_first_avail_block(node, DIRENTRY_SIZE);
}

// returns the first block in the inode which has free space
// or a new block if none do (and adds it to the file)
char *get_first_avail_block(inode *node){
	int ret, total_used_blocks, new_block;
	total_used_blocks = node->filesize / BLOCK_SIZE;
	char *ret;

	ret = (char *)malloc(BLOCKSIZE);
	
	// don't need to return a new block
	if (node->filesize % BLOCK_SIZE != 0){
		if (total_used_blocks < 10){
			// offset by one
			mylseek(fs_disk, node->block->pointers[total_used_blocks], SEEK_SET);
			myread(fs_disk, ret, 1);
		}
		else if (total_used_blocks < 138){
		}
	}
	// need to return a new block
	else{
		new_block = get_new_block();
		mylseek(fs_disk, new_block, SEEK_SET);
		 // TODO
		if (total_used_blocks < 10){
		}
	}
	
	return ret;
}
*/
