CC=gcc
CFLAGS=-c -g -Wall

all : test_fs

test_fs: arraylist.o file_create_delete.o file_dir.o file_init.o file_open_close.o file_read_write.o private_api.o test_fs.o
	$(CC) arraylist.o file_create_delete.o file_dir.o file_init.o file_open_close.o file_read_write.o private_api.o test_fs.o -o test_fs

arraylist.o: arraylist.c
	$(CC) $(CFLAGS) arraylist.c

test_fs.o: test_fs.c
	$(CC) $(CFLAGS) test_fs.c

file_create_delete.o: file_create_delete.c
	$(CC) $(CFLAGS) file_create_delete.c

file_dir.o: file_dir.c
	$(CC) $(CFLAGS) file_dir.c

file_init.o: file_init.c
	$(CC) $(CFLAGS) file_init.c

file_open_close.o: file_open_close.c
	$(CC) $(CFLAGS) file_open_close.c

file_read_write.o: file_read_write.c
	$(CC) $(CFLAGS) file_read_write.c

private_api.o: private_api.c
	$(CC) $(CFLAGS) private_api.c

clean:
	rm -rf *.o test_fs
