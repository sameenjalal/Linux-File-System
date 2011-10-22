/*
 *  api.h
 *  UserspaceFS
 *
 */

// These are the common errors. If you find a need to add others, let me know.
// If they make sense for everyone, I may add them to the common API.
#define ERR_DISK_FULL -1
#define ERR_MAX_FILES -2
#define ERR_FILE_EXISTS -3
#define ERR_PAST_END -4
#define ERR_FILE_NOT_FOUND -5
#define ERR_INVALID_PATH -6
#define ERR_TOO_MANY_FILES_OPEN -7
#define ERR_FILE_NOT_OPEN -8
#define ERR_MAX_FILESIZE -9

#define MAX_FILESIZE 8459264

#define SUCCESS 0
#define LSEEK_FROM_CURRENT 0
#define LSEEK_ABSOLUTE 1
#define LSEEK_END 2

// These functions open the real file on disk that contains your filesystem.

// Opens the real "disk" file. Called once before any other functions are called.
int open_fs(char *fs_path); // DONE

// Closes the "disk" file and synchronizes any unwritten changes.
void close_fs(); // DONE

// Formats the "disk" file with a new file system structure.
int format_fs(char *fs_path, int num_blocks); // DONE

// Opens a file and creates an entry in an "open files" table.
// Returns an int that is in index into this table.
int file_open(char *path); // DONE

// Creates a new file. Does not open the file.
// Returns an error or SUCCESS.
int file_create(char *path); // DONE

// Closes a file. Removes the index int the "open files" table and ensures
// that all data is synchronized with the disk.
void file_close(int file_number); // DONE

// Reads a specified number of bytes from file into buffer.
// Returns the number of bytes actually read.
int file_read(int file_number, void *buffer, int bytes); // DONE

// Writes a specified number of bytes from buffer into file.
// Returns the number of bytes actually written.
int file_write(int file_number, void *buffer, int bytes); // DONE

// Repostition a file's read/write position pointer.
// Takes a command to tell whether to position offset bytes from the current position
// or absolutely.
// If the new offset is past the current end of the file, the file is expanded.
// Returns the new offset or an error.
int file_lseek(int file_number, int offset, int command);

// Deletes the specified file.
// Returns an error or SUCCESS.
int file_delete(char *path); // DONE

// Creates the specified directory.
// Returns an error or SUCCESS.
int file_mkdir(char *path); // DONE

// Removes the specified directory.
// Returns an error or SUCCESS.
int file_rmdir(char *path); // DONE

// Lists the given directory.
// Returns a null pointer terminated array of character strings.
char **file_listdir(char *path); // DONE

// Prints a directory listing to the screen.
void file_printdir(char *path); // DONE
