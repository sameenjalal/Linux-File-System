// Use this to test your filesystem.
// Your program must make sure the filesystem is formatted first.
// You must also call open_fs() first.
// Any error will terminate the test.


#include "api.h"
#include <stdlib.h>
#include <stdio.h>

#define TEST_SET_SIZE 1000000
#define BLOCKS 800
#define DISK_NAME "ssjfs_disk"
void test_fs();

int main(){
	format_fs(DISK_NAME, BLOCKS);
	open_fs(DISK_NAME);
	test_fs();
	close_fs(DISK_NAME);
	return 0;
}

void test_fs()
{
	int return_value;
	int file_number;

	printf("Creating file 1\n");
    return_value = file_create("/test1");
	if(return_value == SUCCESS)
	{
		printf("Successfully created file1...\n\n");
	}
	else
	{
		printf("Could not create file1, failed test%d...\n\n", return_value);
		return;
	}

	printf("Creating file 2\n");
    return_value = file_create("/test2");
	if(return_value == SUCCESS)
	{
		printf("Successfully created file2...\n\n");
	}
	else
	{
		printf("Could not create file2, failed test%d...\n\n", return_value);
		return;
	}

	printf("Creating file 3\n");
    return_value = file_create("/test3");
	if(return_value == SUCCESS)
	{
		printf("Successfully created file3...\n\n");
	}
	else
	{
		printf("Could not create file3, failed test%d...\n\n", return_value);
		return;
	}

    printf("Printing root dir...should see /test1 and /test2...\n");
    file_printdir("/");

	printf("\n\nCREATING directory\n");
    return_value = file_mkdir("/test_dir");
    if(return_value == SUCCESS)
    {
        printf("Successfully created dir...\n");
    }
    else
    {
        printf("Could not create dir, failed test...%d\n", return_value);
    }

    printf("\n\nCREATING file in the directory\n");
	return_value = file_create("/test_dir/test_file");
	if(return_value == SUCCESS)
	{
		printf("Successfully created test_file...\n");
	}
	else
	{
		printf("Could not create file, failed test...%d\n", return_value);
		return;
	}
	printf("opening file /test_dir/test_file\n\n");
	file_number = return_value = file_open("/test_dir/test_file");

	if(return_value >= 0)
	{
		printf("Successfully opened file, file_number = %i...\n", return_value);
	}
	else
	{
		printf("Could not open file, failed test...%d\n", return_value);
		return;
	}

    printf("Doing read/write test...\n");

	printf("Populating array...\n");
	// Generate some test data.

	long unsigned test_array[TEST_SET_SIZE];
	test_array[0] = 0;
	test_array[1] = 1;
	int i;
	for(i=2; i<TEST_SET_SIZE; i++)
	{
		test_array[i] = test_array[i-1] + test_array[i-2];
	}

	printf("Writing array to file...\n");

	for(i=0; i<TEST_SET_SIZE; i++)
	{
		return_value = file_write(file_number, &test_array[i], sizeof(long unsigned));
		if(return_value != sizeof(long unsigned))
		{
			printf("Error while writing...%d\n", return_value);
			return;
		}
	}

	printf("Wrote to file...\n");


    printf("lseeking back to beginning...\n");

    return_value = file_lseek(file_number, 0, LSEEK_ABSOLUTE);
    if(return_value == 0)
    {
        printf("lseek success...\n");
    }
    else
    {
        printf("lseek error...\n");
        return;
    }

	long unsigned* restored_array = malloc(sizeof(long unsigned*)*TEST_SET_SIZE);

	for(i=0; i<TEST_SET_SIZE; i++)
	{
		return_value = file_read(file_number, &restored_array[i], sizeof(long unsigned));
		if(return_value != sizeof(long unsigned))
		{
			printf("Error while reading...\n");
			return;
		}
	}


	printf("Read from file...\n");

	for(i=0; i<TEST_SET_SIZE; i++)
	{
		if(test_array[i] == restored_array[i])
		{
			//printf("Good...test_array[%i] = %lu, restored_array[%i] = %lu...\n", i, test_array[i], i, restored_array[i]);
		}
		else
		{
			printf("Error during compare...test_array[%i] = %lu, restored_array[%i] = %lu...\n", i, test_array[i], i, restored_array[i]);
			return;
		}
	}

  printf("Checked, array good...\n");

    printf("Deleting file...\n");
    return_value = file_delete("/test_dir/test_file");

    if(return_value == SUCCESS)
    {
        printf("Successfully deleted file...\n");
    }
    else
    {
        printf("Error deleting file...\n");
        return;
    }

    printf("Removing dir...\n");
    return_value = file_rmdir("/test_dir");

    if(return_value == SUCCESS)
    {
        printf("Successfully removed dir...\n");
    }
    else
    {
        printf("Error removing dir...\n");
        return;
    }
    printf("Passed basic test...\n");
}
