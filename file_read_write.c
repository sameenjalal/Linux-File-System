// This file contains functions for reading and writing to files

#include "api.h"
#include "private_api.h"

int file_read(int file_number, void *buffer, int bytes)
{
	// Get file from array_list
	file_info *file = arraylist_get(file_number, open_files);
	if ( !file )
		return ERR_FILE_NOT_FOUND;
	int inum = file->inumber;
	char *path = file->filepath;
	int cursor = (file->cursor);

	// the current inode
	inode *node = get_inode(inum);
	if ( !node )
		return ERR_FILE_NOT_FOUND;

	int size_of_file = node->filesize;
	int bytes_actually_read;
	if ( size_of_file - cursor < bytes )
		bytes_actually_read = size_of_file - cursor;
	else
		bytes_actually_read = bytes;

	int block_num = cursor / BLOCK_SIZE;
	int cursor_in_block = cursor % BLOCK_SIZE;
	int absolute_block = get_absolute_block(inum, block_num);

	char my_buffer[BLOCK_SIZE];

	// if the cursor starts and ends in one block
	if ( cursor_in_block + bytes_actually_read < BLOCK_SIZE )
	{
		mylseek(fs_disk, absolute_block, SEEK_SET);
		myread(fs_disk, my_buffer, 1);

		memcpy(buffer, &my_buffer[cursor_in_block], bytes_actually_read);
	}
	// cursor starts in current block and ends somewhere elsem
	else // ( !(cursor_in_block + bytes_actually_read < 512) )
	{
		// [__||]
		int bytes_read_from_first_block = BLOCK_SIZE - cursor_in_block;

		// [||||][||__]
		int bytes_to_still_read = bytes_actually_read - bytes_read_from_first_block;

		// [||__]
		int bytes_to_read_in_last_block = bytes_to_still_read % BLOCK_SIZE;

		// [||||][||||]
		int bytes_to_read_from_middle = bytes_actually_read - (bytes_read_from_first_block + bytes_to_read_in_last_block);

		// first block
		mylseek(fs_disk, absolute_block, SEEK_SET);
		myread(fs_disk, my_buffer, 1);
		char buffer_begin[bytes_read_from_first_block];
		memcpy(buffer_begin, &my_buffer[BLOCK_SIZE - bytes_read_from_first_block], bytes_read_from_first_block);

		// second block
		int last_block_num = (cursor + bytes_actually_read) / BLOCK_SIZE;
		absolute_block = get_absolute_block(inum, last_block_num);
		mylseek(fs_disk, absolute_block, SEEK_SET);
		myread(fs_disk, my_buffer, 1);
		char buffer_end[bytes_to_read_in_last_block];
		memcpy(buffer_end, my_buffer, bytes_to_read_in_last_block);

		// Where the num bytes to read only spans another block [__||][||__]
		if ( bytes_to_read_from_middle == 0 )
		{
			// join both begining and end buffers into the output buffer
			buffer = (char *) malloc( sizeof(char)*(strlen(buffer_begin) + strlen(buffer_end)) );
			memcpy( buffer, buffer_begin, strlen(buffer_begin) );
			memcpy( (char *) &buffer[ bytes_read_from_first_block + 1 ], buffer_end, bytes_to_read_in_last_block );
		}
		else // Where the num bytes to read spans more than one block [_||||][||||][||||][||__]
		{
			// Copy each middle block into a buffer
			int num_blocks_to_still_read = bytes_to_read_from_middle / BLOCK_SIZE;
			char middle_buffer[BLOCK_SIZE*num_blocks_to_still_read];
			int i;
			for ( i = block_num + 1 ; i < last_block_num ; i++ )
			{
				absolute_block = get_absolute_block(inum, i);
				mylseek(fs_disk, absolute_block, SEEK_SET);
				myread(fs_disk, my_buffer, 1);

				int current_position_in_middle_buffer = (i - 1 - block_num) * BLOCK_SIZE;
				memcpy(&middle_buffer[current_position_in_middle_buffer], my_buffer, BLOCK_SIZE);
			}
			buffer = (char *) malloc(sizeof(char) * (strlen(buffer_begin)+strlen(middle_buffer)+strlen(buffer_end)) );
			memcpy( buffer, buffer_begin, strlen(buffer_begin) );
			memcpy( &buffer[strlen(buffer_begin)+1], middle_buffer, strlen(middle_buffer) );
			memcpy( &buffer[strlen(buffer_begin) + strlen(middle_buffer) + 1], buffer_end, strlen(buffer_end) );
		}
	}

	// Increment cursor in file as you read that
	// many bytes and set it back in the arraylist
	file_info *new_file = (file_info *) malloc(sizeof(file_info));
	strcpy(new_file->filepath, path);
	new_file->inumber = inum;
	new_file->cursor = cursor + bytes_actually_read;

	arraylist_set(file_number, new_file, open_files);
	return bytes_actually_read;
}

int file_write(int file_number, void *buffer, int bytes)
{
	// Get file from array_list
	file_info *file = arraylist_get(file_number, open_files);
	if ( !file )
		return ERR_FILE_NOT_FOUND;
	int inum = file->inumber;
	char *path = file->filepath;
	int cursor = (file->cursor);

	// the current inode
	inode *node = get_inode(inum);
	if ( !node )
		return ERR_FILE_NOT_FOUND;

	int size_of_file = node->filesize;
	int bytes_actually_written;
	if ( cursor + bytes >= MAX_FILESIZE )
		bytes_actually_written = MAX_FILESIZE - cursor;
	else
		bytes_actually_written = bytes;

	int block_num = cursor / BLOCK_SIZE;
	int cursor_in_block = cursor % BLOCK_SIZE;
	int absolute_block = get_absolute_block(inum, block_num);

	char my_buffer[BLOCK_SIZE];

	// writing within the filesize
	if ( cursor + bytes_actually_written <= size_of_file )
	{
		// if the cursor starts and ends in one block
		if ( cursor_in_block + bytes_actually_written < BLOCK_SIZE )
		{
			mylseek(fs_disk, absolute_block, SEEK_SET);
			myread(fs_disk, my_buffer, 1);

			memcpy(my_buffer, &buffer[cursor_in_block], bytes_actually_written);
			mywrite(fs_disk, my_buffer, 1);
		}
		// cursor starts in current block and ends somewhere else
		else // ( !(cursor_in_block + bytes_actually_read < 512) )
		{
			// [__||]
			int bytes_written_from_first_block = BLOCK_SIZE - cursor_in_block;

			// [||||][||__]
			int bytes_to_still_write = bytes_actually_written - bytes_written_from_first_block;

			// [||__]
			int bytes_to_write_in_last_block = bytes_to_still_write % BLOCK_SIZE;

			// [||||][||||]
			int bytes_to_write_from_middle = bytes_actually_written - (bytes_written_from_first_block + bytes_to_write_in_last_block);

			// first block
			mylseek(fs_disk, absolute_block, SEEK_SET);
			myread(fs_disk, my_buffer, 1);
			memcpy(my_buffer, buffer, bytes_written_from_first_block);
			mywrite(fs_disk, my_buffer, 1);

			// second block
			int last_block_num = (cursor + bytes_actually_written) / BLOCK_SIZE;
			absolute_block = get_absolute_block(inum, last_block_num);
			mylseek(fs_disk, absolute_block, SEEK_SET);
			myread(fs_disk, my_buffer, 1);
			int starting_position_in_last_block = bytes_written_from_first_block + bytes_to_write_from_middle;
			memcpy(my_buffer, &buffer[starting_position_in_last_block], bytes_to_write_in_last_block);
			mywrite(fs_disk, my_buffer, 1);

			// Where the num bytes to write only spans another block [__||][||__]
			if ( bytes_to_write_from_middle == 0 )
			{
				// Do nothing
			}
			else // Where the num bytes to write spans more than one block [_||||][||||][||||][||__]
			{
				// Copy each middle block into a buffer
				//int num_blocks_to_still_write = bytes_to_write_from_middle / BLOCK_SIZE;
				int i;
				for ( i = block_num + 1 ; i < last_block_num ; i++ )
				{
					absolute_block = get_absolute_block(inum, i);
					mylseek(fs_disk, absolute_block, SEEK_SET);
					myread(fs_disk, my_buffer, 1);

					int starting_position_in_buffer = bytes_written_from_first_block + (i - block_num - 1) * BLOCK_SIZE;
					memcpy(my_buffer, &buffer[starting_position_in_buffer], BLOCK_SIZE);
					mywrite(fs_disk, my_buffer, 1);
				}
			}
		}
		// Do not have to increase filesize

		// Increment cursor in file as you write in that
		// many bytes and set it back in the arraylist
		file_info *new_file = (file_info *) malloc(sizeof(file_info));
		strcpy(new_file->filepath, path);
		new_file->inumber = inum;
		new_file->cursor = cursor + bytes_actually_written;

		arraylist_set(file_number, new_file, open_files);
	}
	else // must increase filesize
	{
		// do the same as above except this time I increase filesize
		// which means I might have to get a new block
		// Do not have to worry about going past MAX_FILESIZE
		int new_file_size = cursor + bytes_actually_written;

		// CASE 1: Go past current file but within same block
		// if the cursor starts and ends in one block
		if ( cursor_in_block + bytes_actually_written < BLOCK_SIZE )
		{
			mylseek(fs_disk, absolute_block, SEEK_SET);
			myread(fs_disk, my_buffer, 1);

			memcpy(&my_buffer[cursor_in_block], buffer, bytes_actually_written);
			mywrite(fs_disk, my_buffer, 1);
		}
		// CASE 2: Go past current file and past current block
		// Cursor starts in current block and ends somewhere else
		else // ( !(cursor_in_block + bytes_actually_read < 512) )
		{
			// [__||]
			int bytes_written_from_first_block = BLOCK_SIZE - cursor_in_block;

			// [||||][||__]
			int bytes_to_still_write = bytes_actually_written - bytes_written_from_first_block;

			// [||__]
			int bytes_to_write_in_last_block = bytes_to_still_write % BLOCK_SIZE;

			// [||||][||||]
			int bytes_to_write_from_middle = bytes_actually_written - (bytes_written_from_first_block + bytes_to_write_in_last_block);

			// first block
			mylseek(fs_disk, absolute_block, SEEK_SET);
			myread(fs_disk, my_buffer, 1);
			memcpy(my_buffer, buffer, bytes_written_from_first_block);
			mywrite(fs_disk, my_buffer, 1);

			// Where the num bytes to write only spans another block [__||][||__]
			if ( bytes_to_write_from_middle == 0 )
			{
				// bring in new block and write to it
				int new_block = get_new_block();
				add_block_to_inode(node, new_block);

				// use new block to write to
				absolute_block = get_absolute_block(inum, new_block);
				mylseek(fs_disk, absolute_block, SEEK_SET);
				myread(fs_disk, my_buffer, 1);
				int starting_position_in_last_block = bytes_to_write_in_last_block + bytes_written_from_first_block;
				memcpy(my_buffer, &buffer[starting_position_in_last_block], bytes_to_write_in_last_block);
				mywrite(fs_disk, my_buffer, 1);

				write_inode(node, inum);
			}

			else // Where the num bytes to write spans more than one block [_||||][||||][||||][||__]
			{
				// First check how many bytes we are still able to write within the file
				// Have to run out of filesize sometime so until there is still filesize, keep writing
				// Then I can know for sure the rest of the memory has to be added

				int current_cursor_after_first_block_written_to = size_of_file - cursor - bytes_written_from_first_block;
				int remaining_file_size = size_of_file - current_cursor_after_first_block_written_to;
				int num_blocks_left = remaining_file_size / BLOCK_SIZE;
				int k, total_bytes_written_in_remaining_memory = bytes_written_from_first_block;
				for ( k = 0 ; k < num_blocks_left ; k++ )
				{
					absolute_block = get_absolute_block(inum, k + block_num + 1);
					mylseek(fs_disk, absolute_block, SEEK_SET);
					myread(fs_disk, my_buffer, 1);

					int starting_position_in_buffer = bytes_written_from_first_block + k * BLOCK_SIZE;
					memcpy(my_buffer, &buffer[starting_position_in_buffer], BLOCK_SIZE);
					mywrite(fs_disk, my_buffer, 1);
					total_bytes_written_in_remaining_memory += BLOCK_SIZE;
				}

				// How many new blocks to bring in
				int num_blocks_to_bring_in = (new_file_size - size_of_file) / BLOCK_SIZE;
				int j;
				for ( j = 0 ; j < num_blocks_to_bring_in ; j++ )
				{
					int new_block = get_new_block();
					int failed_to_get_more_memory = add_block_to_inode(node, new_block);
					if ( failed_to_get_more_memory )
					{
						return ERR_MAX_FILESIZE;
					}
					absolute_block = get_absolute_block(inum, new_block);
					mylseek(fs_disk, absolute_block, SEEK_SET);
					myread(fs_disk, my_buffer, 1);

					int starting_position_in_buffer = total_bytes_written_in_remaining_memory + j * BLOCK_SIZE;
					if ( strlen(buffer) - starting_position_in_buffer > 511 )
					{
						memcpy(my_buffer, &buffer[starting_position_in_buffer], BLOCK_SIZE);
						mywrite(fs_disk, my_buffer, BLOCK_SIZE);
					}
					else
					{
						int remaining_buffer = strlen(buffer) - starting_position_in_buffer;
						memcpy( my_buffer, &buffer[starting_position_in_buffer], remaining_buffer );
						mywrite(fs_disk, my_buffer, remaining_buffer);
					}
					write_inode(node, inum);
				}
			}
		}
		// Have to increase filesize


		// Increment cursor in file as you write in that
		// many bytes and set it back in the arraylist
		file_info *new_file = (file_info *) malloc(sizeof(file_info));
		strcpy(new_file->filepath, path);
		new_file->inumber = inum;
		new_file->cursor = cursor + bytes_actually_written;

		arraylist_set(file_number, new_file, open_files);
		node->filesize += new_file->cursor;
		write_inode(node, inum);
	}
	return bytes_actually_written;
}

int file_lseek(int filenumber, int offset, int command)
{
	// Get file from array_list
	file_info *file = arraylist_get(filenumber, open_files);
	if ( !file )
		return ERR_FILE_NOT_FOUND;
	int inum = file->inumber;
	char *path = file->filepath;
	int cursor = (file->cursor);

	// the current inode
	inode *node = get_inode(inum);
	if ( !node )
		return ERR_FILE_NOT_FOUND;

	int size_of_file = node->filesize;
	int bytes_actually_moved;

	// command of 1 means to go relatively
	if ( command == LSEEK_FROM_CURRENT )
	{
		if ( cursor + offset >= MAX_FILESIZE )
			bytes_actually_moved = MAX_FILESIZE - cursor;
		else
			bytes_actually_moved = offset;

		// CASE 1: bytes_actually_moved moves within current filesize
		if ( cursor + bytes_actually_moved <= size_of_file )
		{
			file_info *new_file = (file_info *) malloc(sizeof(file_info));
			strcpy(new_file->filepath, path);
			new_file->inumber = inum;
			new_file->cursor = cursor + bytes_actually_moved;

			arraylist_set(filenumber, new_file, open_files);
			return new_file->cursor;
		}
		else // CASE 2: bytes_actually_moved moves outside current filesize
		{
			// CASE 1: if moved outside the filesize but within the last block
			int block_num = cursor / BLOCK_SIZE;
			int ending_block_num = (cursor + bytes_actually_moved) / BLOCK_SIZE;
			if ( ending_block_num == block_num )
			{
				// put new file info back into arraylist of open files
				file_info *new_file = (file_info *) malloc(sizeof(file_info));
				strcpy(new_file->filepath, path);
				new_file->inumber = inum;
				new_file->cursor = cursor + bytes_actually_moved;

				arraylist_set(filenumber, new_file, open_files);

				// increment file size and peace out
				node->filesize = cursor + bytes_actually_moved;
				write_inode(node, inum);

				return new_file->cursor;
			}

			// CASE 2: if moved outside the filesize but outside the last block
			// meaning we have to get more blocks
			else
			{
				int b = ending_block_num - block_num;
				for ( ; b >= 0 ; b-- )
				{
					int new_block_number = get_new_block();
					add_block_to_inode(node, new_block_number);
				}

				// write the new filesize back to disk
				int new_file_size = cursor + bytes_actually_moved;
				node->filesize = new_file_size;
				write_inode(node, inum);

				// put new file info back into arraylist of open files
				file_info *new_file = (file_info *) malloc(sizeof(file_info));
				strcpy(new_file->filepath, path);
				new_file->inumber = inum;
				new_file->cursor = new_file_size;

				arraylist_set(filenumber, new_file, open_files);

				// peace out
				return new_file->cursor;
			}
		}
	}
	else if ( command == LSEEK_ABSOLUTE ) // command of anything else means absolutely
	{
		if ( offset > MAX_FILESIZE )
			bytes_actually_moved = MAX_FILESIZE;
		else
			bytes_actually_moved = offset;

		// CASE 1: bytes_actually_moved moves within current filesize
		if ( bytes_actually_moved <= size_of_file )
		{
			file_info *new_file = (file_info *) malloc(sizeof(file_info));
			strcpy(new_file->filepath, path);
			new_file->inumber = inum;
			new_file->cursor = bytes_actually_moved;

			arraylist_set(filenumber, new_file, open_files);
			return new_file->cursor;
		}
		else // CASE 2: bytes_actually_moved moves outside current filesize
		{
			// CASE 1: if moved outside the filesize but within the last block
			int block_num = cursor / BLOCK_SIZE;
			int ending_block_num = (bytes_actually_moved) / BLOCK_SIZE;
			if ( ending_block_num == block_num )
			{
				// put new file info back into arraylist of open files
				file_info *new_file = (file_info *) malloc(sizeof(file_info));
				strcpy(new_file->filepath, path);
				new_file->inumber = inum;
				new_file->cursor = bytes_actually_moved;

				arraylist_set(filenumber, new_file, open_files);

				// increment file size and peace out
				node->filesize = bytes_actually_moved;
				write_inode(node, inum);

				return new_file->cursor;
			}

			// CASE 2: if moved outside the filesize but outside the last block
			// meaning we have to get more blocks
			else
			{
				int b = ending_block_num - block_num;
				for ( ; b >= 0 ; b-- )
				{
					int new_block_number = get_new_block();
					add_block_to_inode(node, new_block_number);
				}

				// write the new filesize back to disk
				int new_file_size = bytes_actually_moved;
				node->filesize = new_file_size;
				write_inode(node, inum);

				// put new file info back into arraylist of open files
				file_info *new_file = (file_info *) malloc(sizeof(file_info));
				strcpy(new_file->filepath, path);
				new_file->inumber = inum;
				new_file->cursor = new_file_size;

				arraylist_set(filenumber, new_file, open_files);

				// peace out
				return new_file->cursor;
			}
		}
	}
	else // if ( command == LSEEK_END )
	{
		if ( offset > MAX_FILESIZE )
			bytes_actually_moved = MAX_FILESIZE;
		else
			bytes_actually_moved = offset;

		// CASE 1: if moved outside the filesize but within the last block
		int block_number = cursor / BLOCK_SIZE;
		int ending_block_num = (bytes_actually_moved) / BLOCK_SIZE;
		if ( ending_block_num == block_number )
		{
			// put new file info back into arraylist of open files
			file_info *new_file = (file_info *) malloc(sizeof(file_info));
			strcpy(new_file->filepath, path);
			new_file->inumber = inum;
			new_file->cursor = size_of_file + bytes_actually_moved;

			arraylist_set(filenumber, new_file, open_files);

			// increment file size and peace out
			node->filesize = size_of_file + bytes_actually_moved;
			write_inode(node, inum);

			return new_file->cursor;
		}

		// CASE 2: if moved outside the filesize but outside the last block
		// meaning we have to get more blocks
		else
		{
			int b = ending_block_num - block_number;
			for ( ; b >= 0 ; b-- )
			{
				int new_block_number = get_new_block();
				add_block_to_inode(node, new_block_number);
			}

			// write the new filesize back to disk
			int new_file_size = size_of_file + bytes_actually_moved;
			node->filesize = new_file_size;
			write_inode(node, inum);

			// put new file info back into arraylist of open files
			file_info *new_file = (file_info *) malloc(sizeof(file_info));
			strcpy(new_file->filepath, path);
			new_file->inumber = inum;
			new_file->cursor = size_of_file + new_file_size;

			arraylist_set(filenumber, new_file, open_files);

			// peace out
			return new_file->cursor;
		}
	}
}
