#include "api.h"
#include "private_api.h"

// This function opens the file system by initializing a a global file
// desc using the specified path and "open". The global fs_disk is in the api.h file
int
open_fs(char *fs_path)
{
    if ( (fs_disk = open(fs_path, O_RDWR | O_NONBLOCK)) )
    {
        open_files = arraylist_new(10);
		printf("open_files list : %p\n", open_files);
        return 0;
    }
    else
    {
        fprintf(stderr, "Filesystem could not be opened!\n");
        return 1;
    }
}

// close_fs uses "close" on the global file descriptor
void
close_fs()
{
    arraylist_destroy(open_files);
    int status = close(fs_disk);
    if ( status != 0 )
    {
        fprintf(stderr, "Filesystem could not be closed!\n");
        exit(1);
    }
}

//Take a path, open it to get a global file descriptor and format it
//for num_blocks blocks
int
format_fs(char *fs_path, int num_blocks)
{
	//printf("Entering formatfs\n");
	superblock *sb;
	inode *iptr;
	free_block *bptr;
	char buffer[BLOCK_SIZE];
	int total_inode_blocks;
	int block_pointer, dbn;
	int i;

	//printf("Opening fs disk\n");
    fs_disk = open(fs_path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	//printf("fs_disk set to %d\n", fs_disk);
	if (fs_disk == -1)
    {
        fprintf(stderr, "Filesystem could not be opened for creation!\n");
        exit(1);
    }
	//printf("disk opened!\n");
	// Build the superblock!
	sb = (superblock *)malloc(sizeof(superblock));
	strcpy(sb->fs_type, FILESYSTEM_NAME);
	sb->block_count = num_blocks;
	sb->current_files = 0;

	// Max files, also inode count, will be based on the amount of overhead we specified
	// and will be the number of inodes that can fit in the number of blocks we get for them
	total_inode_blocks = num_blocks / INODE_OVERHEAD;
	sb->max_files = total_inode_blocks * (BLOCK_SIZE / INODE_SIZE);

	// Allocated blocks will be the superblock + the blocks reserved for inodes
	sb->allocated_blocks = 1 + total_inode_blocks;

	// first free data block will be the first block after the inode blocks
	sb->first_free_block = 1 + total_inode_blocks;
	sb->first_free_inode = 0;

/*	printf("total_inode blocks: %d\n"
			"last inode block number: %d\n"
			"first_free_block: %d\n",
			total_inode_blocks,
			total_inode_blocks,
			total_inode_blocks+1);
*/
	// save changes to superblock
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);

	// Now, go build inodes
	//printf("Building %d inodes\n", sb->max_files);
	for (i = 0; i < sb->max_files; i++){
		// Have to build each inode.  On every (block size / inode size) node, write them to
		// disk and move to the next disk.  Each inode points to the next one (and prev pointers)
		iptr = (inode *)malloc(INODE_SIZE);

		// Initialize values
		iptr->filesize = 0;
		iptr->directory_flag = 0;
		if (i == sb->max_files - 1) iptr->next_free = 0;
		else iptr->next_free = i + 1;
		if (i == 0) iptr->prev_free = sb->max_files - 1;
		else iptr->prev_free = i - 1;
	//	printf("Writing %d and %d into %d\n", iptr->prev_free, iptr->next_free, i);
		
		write_inode(iptr, i);
		free(iptr);

		/*
		// Now write to disk if this is the last inode in the block
		if (i % (BLOCK_SIZE / INODE_SIZE) == 0){
			mywrite(fs_disk, buffer, 1);
			mylseek(fs_disk, 1, SEEK_CUR);
		}
		*/
	}
	/*
	for (i = 0; i < 5; i++){
		iptr = get_inode(i);
		printf("TTESSSSTTINGNI i = %d prev = %d next = %d\n", i, iptr->prev_free, iptr->next_free);
	}
	*/
	// Then, build empty data blocks.  Set all statuses to open and each pointer to the next block,
	// wrap the last one back around to the first, and don't forgot prev pointers.
//	printf("Building free datablocks\n");
	for (block_pointer = sb->allocated_blocks; block_pointer < sb->block_count; block_pointer++){
		bptr = (free_block *)buffer;
		mylseek(fs_disk, block_pointer, SEEK_SET);
		// first set it all to zero
		memset(bptr, 0, BLOCK_SIZE);
		if (block_pointer == sb->allocated_blocks) bptr->prev_free = sb->block_count -1;
		else bptr->prev_free = block_pointer - 1 ;

		if (block_pointer == sb->block_count - 1) bptr->next_free = sb->allocated_blocks;
		bptr->next_free = block_pointer + 1;
		// write the block to disk
		//printf("writing block %d: pointers are %d and %d\n", block_pointer, bptr->prev_free, bptr->next_free);
		mywrite(fs_disk, bptr, 1);
	}
	for (block_pointer = sb->allocated_blocks; block_pointer < sb->allocated_blocks + 5; block_pointer++){
		bptr = load_free_block(block_pointer);
//		printf("ASLDAJDA bptr->next = %d bptr->prev = %d\n", bptr->next_free, bptr->prev_free);
	}
//	printf("Done building free datablocks\n");
	
	// Ok now write the superblock to disk
//	printf("Seeking to fs_disk - %d\n", fs_disk);
	mylseek(fs_disk, 0, SEEK_SET);
	mywrite(fs_disk, sb, 1);
	//printf("Done writing superblock back\n");

	// finally, make root directory
	i = get_new_inode();
	if (i != ROOT_INODE){
		fprintf(stderr, "get_new_inode() didnt return 0 on the format call?");
		return ERR_FILE_NOT_FOUND; // for some reason lulz
	}
	iptr = get_inode(i);
	iptr->directory_flag = 1;
	write_inode(iptr, i);

	/*
	// grab the block we're going to write to so we can get the next free block
	bptr = load_free_block(block_pointer);
	nfb = bptr->next_free;

	// grab previous block so we can change its pointer
	prev_block = bptr->prev_free;
	prev_bptr = get_free_block(prev_block);

	// grab next block so we can change its previous pointer
	next_bptr = load_free_block(nfb);

	// set pointers
	prev_bptr->next_free = nfb;
	sb->first_free_block = nfb;
	next_bptr->prev_free = prev_block;

	// write empty root directory block
	mylseek(fs_disk, block_pointer, SEEK_SET);
	memset(bptr, 0, BLOCK_SIZE);
	mywrite(fs_disk, bptr, 1);

	// write other two blocks
	mylseek(fs_disk, nfb, SEEK_SET);
	mywrite(fs_disk, next_bptr, 1);

	mylseek(fs_disk, prev_block, SEEK_SET);
	mywrite(fs_disk, prev_bptr, 1);

	// point root inode at the block we wrote.
	iptr->block_pointers[0] = block_pointer;
	iptr->status = 1;

	// update inode list stuff
	inmbr = 0;
	i_prev = iptr->prev_free;
	i_next = iptr->next_free;

	iprev = get_inode(i_prev);
	inext = get_inode(i_next);

	iprev->next_free = iptr->next_free;
	inext->prev_free = iptr->prev_free;
	sb->first_free_inode = iptr->next_free;

	// Write inodes back to disk
	write_inode(iptr, 0);
	write_inode(inext, iptr->next_free);
	write_inode(iprev, iptr->prev_free);
	*/

	/*
	// update the superblock
	sb->free_blocks--;
	sb->allocated_blocks++;
	sb->current_files++;
	mylseek(fs_disk, 0, SEEK_SET):
	mywrite(fs_disk, sb, 1);
	*/

	// Cleanup
	/*
	free(iptr);
	free(inext);
	free(iprev);
	free(bptr);
	free(prev_bptr);
	free(next_bptr);
	*/
	free(sb);
	close(fs_disk);
	printf("Exiting formatfs\n");
	return 0;
}
