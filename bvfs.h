/* CMSC 432 - Homework 7
 * Assignment Name: bvfs - the BV File System
 * Due: Thursday, November 21st @ 11:59 p.m.
 */


/*
 * [Requirements / Limitations]
 *   Partition/Block info
 *     - Block Size: 512 bytes
 *     - Partition Size: 8,388,608 bytes (16,384 blocks)
 *
 *   Directory Structure:
 *     - All files exist in a single root directory
 *     - No subdirectories -- just names files
 *
 *   File Limitations
 *     - File Size: Maximum of 65,536 bytes (128 blocks)
 *     - File Names: Maximum of 32 characters including the null-byte
 *     - 256 file maximum -- Do not support more
 *
 *   Additional Notes
 *     - Create the partition file (on disk) when bv_init is called if the file
 *       doesn't already exist.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Prototypes
int bv_init(const char *fs_fileName);
int bv_destroy();
int bv_open(const char *fileName, int mode);
int bv_close(int bvfs_FD);
int bv_write(int bvfs_FD, const void *buf, size_t count);
int bv_read(int bvfs_FD, void *buf, size_t count);
int bv_unlink(const char* fileName);
void bv_ls();

// max of 256 iNodes
struct iNode{
    char fileName[32]; // 32 bytes
    int size; // 4 bytes
    int time; // 4 bytes
    short address[128]; // 256 bytes
};

// Global variables
const int BLOCK_SIZE = 512;
const int PARTITION_SIZE = 8388608;
const int MAX_BLOCKS = 16384;
const int MAX_FILE_SIZE = 65536;
const int MAX_FILE_BLOCKS = 128;
const int MAX_FILES = 256;
const int MAX_FILE_NAME = 32;
const int INODE_START = 512;
int GLOBAL_PFD;
int INIT_FLAG = 0;
short superblock_array[256];
struct iNode inode_arr[256];

/*
 * int bv_init(const char *fs_fileName);
 *
 * Initializes the bvfs file system based on the provided file. This file will
 * contain the entire stored file system. Invocation of this function will do
 * one of two things:
 *
 *   1) If the file (fs_fileName) exists, the function will initialize in-memory
 *   data structures to help manage the file system methods that may be invoked.
 *
 *   2) If the file (fs_fileName) does not exist, the function will create that
 *   file as the representation of a new file system and initialize in-memory
 *   data structures to help manage the file system methods that may be invoked.
 *
 * Input Parameters
 *   fs_fileName: A c-string representing the file on disk that stores the bvfs
 *   file system data.
 *
 * Return Value
 *   int:  0 if the initialization succeeded.
 *        -1 if the initialization failed (eg. file not found, access denied,
 *           etc.). Also, print a meaningful error to stderr prior to returning.
 */
int bv_init(const char *fs_fileName) {
  // try to open file
  int pFD = open(fs_fileName, O_CREAT | O_RDWR | O_EXCL, 0644);
    if (pFD < 0) {
        if (errno == EEXIST) {
            // file already exists
            printf("File already exists.\n");
            pFD = open(fs_fileName, O_CREAT | O_RDWR , S_IRUSR | S_IWUSR);
            GLOBAL_PFD = pFD;
            INIT_FLAG = 1;

            // open in memory data structures
            // read in structures
            // superblock, inodes

            lseek(pFD, 0, SEEK_SET);
            int sblock_start;
            read(pFD, &sblock_start, 4);
            printf("Read super block pos: %d\n",sblock_start);

            // Initialize superblock array looking at first superblock
            // Should get the first superblock that contains at least one usable offset
            printf("sblock start %d\n", sblock_start);
            short temp = 0;
            int ctr = 0;
            short sarr[256];
            while(temp == 0){
              lseek(pFD, sblock_start, SEEK_SET);
                for (short i = 0; i < 256; i++){
                    read(pFD, &temp, 2);
                    sarr[ctr] = temp;
                    ctr++;
                    if(temp != 0) // Asserts one usable offset
                      break;
                }
              if(temp == 0)
                sblock_start = sarr[255] * BLOCK_SIZE;
            }

            printf("stblock start %d\n", sblock_start);
            lseek(pFD, sblock_start, SEEK_SET);
            for(short i = 0; i < 256; i++){
              read(pFD, &temp, 2);
              superblock_array[i] = temp;
            }

            //lseek(pFD, INODE_START, SEEK_SET);
            //read(pFD, &arr, sizeof(arr));

            close(pFD);
            return 0;
        }
        else {
            // Something bad must have happened... check errno?
            printf("%s\n", strerror(errno));
            return -1;
        }

    } else {
        // File did not previously exist but it does now. Write data to it
        GLOBAL_PFD = pFD;
        INIT_FLAG = 1;
        char nbyte = '\0';
        // seek to position of max parition size - 1 and then write 0 to signify end of file
        lseek(pFD, PARTITION_SIZE-1, SEEK_SET);
        write(pFD, (void*)&nbyte, 1);
        printf("Created File and wrote zero at the end.\n");

        // create in memory data structures
        // read in structures
        // superblock, inode 

        // 512 bytes for "super block" pointer in beginning
        // 75,776 bytes for inodes
        // 76,288 bytes for meta data

        // 8,388,608 bytes total
        // minus 76,288 meta data bytes
        // leaves 8,312,320 bytes for data region
        /*
           8,312,320 divided by 512 is 16,235 blocks that need managed.
           To point to these blocks is going to require 16,235 * 2 bytes (shorts) = 32,470 bytes
           32,470 divided by 512 is ~64 blocks pointing to data blocks
         */
        /*
           Position to "super block" points to = 76,288th byte.
           This super block can point to the next 255 blocks.
           256th pointer points to the next super block in the data region.
         */

        // Get block num and write it to file
        struct iNode test = {"hello\n", 1, 1, 0};
        arr[0] = test;
        struct iNode dummy = {"hello dummy\n", 1, 1, 0};
        arr[200] = dummy;

        int endOfMeta = 76288;
        lseek(pFD, 0, SEEK_SET); // Seek to 0
        write(pFD, (void*)(&endOfMeta), 4); // Write the start of the super block linked list

        // Seek to end of first superblock
        lseek(pFD, INODE_START, SEEK_SET);
        write(pFD, (void*)(&inode_arr), sizeof(inode_arr));

        // Go to start of linked list and piece it together
        lseek(pFD, 76288, SEEK_SET);
        // Write addresses to next 256 blocks
        short blockNum = (76288/BLOCK_SIZE);
        while (blockNum < MAX_BLOCKS) {
            for (short i = blockNum+1; i<=(blockNum+256);i++) {
              if(i*BLOCK_SIZE<PARTITION_SIZE-1)
                write(pFD, (void*)&i, 2);
              else
                write(pFD, "\0\0", 2);
            }
            //printf("block num : %d\n",blockNum);
            blockNum += 256;
            lseek(pFD, blockNum * BLOCK_SIZE, SEEK_SET);
        }
        // Need to add further super blocks to for loop

        close(pFD);
        return 0;
    }
}






/*
 * int bv_destroy();
 *
 * This is your opportunity to free any dynamically allocated resources and
 * perhaps to write any remaining changes to disk that are necessary to finalize
 * the bvfs file before exiting.
 *
 * Return Value
 *   int:  0 if the clean-up process succeeded.
 *        -1 if the clean-up process failed (eg. bv_init was not previously,
 *           called etc.). Also, print a meaningful error to stderr prior to
 *           returning.
 */
int bv_destroy() {
  // Write the iNode array back to file to save changes
  if(INIT_FLAG == 1){
    // File has been initialized.
    lseek(GLOBAL_PFD, INODE_START, SEEK_SET);
    write(GLOBAL_PFD, (void*)(&inode_arr), sizeof(inode_arr));
    close(GLOBAL_PFD);
  }
  else{
    // File has not been initialized.
    char s[] = "File cleanup unsuccesful.\0";
    write(2, (void*)(s), sizeof(s));
    return -1;
  }
}







// Available Modes for bvfs (see bv_open below)
int BV_RDONLY = 0;
int BV_WCONCAT = 1;
int BV_WTRUNC = 2;

/*
 * int bv_open(const char *fileName, int mode);
 *
 * This function is intended to open a file in either read or write mode. The
 * above modes identify the method of access to utilize. If the file does not
 * exist, you will create it. The function should return a bvfs file descriptor
 * for the opened file which may be later used with bv_(close/write/read).
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to fetch
 *             (or create) in the bvfs file system.
 *   mode: The access mode to use for accessing the file
 *           - BV_RDONLY: Read only mode
 *           - BV_WCONCAT: Write only mode, appending to the end of the file
 *           - BV_WTRUNC: Write only mode, replacing the file and writing anew
 *
 * Return Value
 *   int: >=0 Greater-than or equal-to zero value representing the bvfs file
 *           descriptor on success.
 *        -1 if some kind of failure occurred. Also, print a meaningful error to
 *           stderr prior to returning.
 */
int bv_open(const char *fileName, int mode) {
}






/*
 * int bv_close(int bvfs_FD);
 *
 * This function is intended to close a file that was previously opened via a
 * call to bv_open. This will allow you to perform any finalizing writes needed
 * to the bvfs file system.
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to fetch
 *             (or create) in the bvfs file system.
 *
 * Return Value
 *   int:  0 if open succeeded.
 *        -1 if some kind of failure occurred (eg. the file was not previously
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_close(int bvfs_FD) {
}







/*
 * int bv_write(int bvfs_FD, const void *buf, size_t count);
 *
 * This function will write count bytes from buf into a location corresponding
 * to the cursor of the file represented by bvfs_FD.
 *
 * Input Parameters
 *   bvfs_FD: The identifier for the file to write to.
 *   buf: The buffer containing the data we wish to write to the file.
 *   count: The number of bytes we intend to write from the buffer to the file.
 *
 * Return Value
 *   int: >=0 Value representing the number of bytes written to the file.
 *        -1 if some kind of failure occurred (eg. the file is not currently
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_write(int bvfs_FD, const void *buf, size_t count) {
}






/*
 * int bv_read(int bvfs_FD, void *buf, size_t count);
 *
 * This function will read count bytes from the location corresponding to the
 * cursor of the file (represented by bvfs_FD) to buf.
 *
 * Input Parameters
 *   bvfs_FD: The identifier for the file to read from.
 *   buf: The buffer that we will write the data to.
 *   count: The number of bytes we intend to write to the buffer from the file.
 *
 * Return Value
 *   int: >=0 Value representing the number of bytes written to buf.
 *        -1 if some kind of failure occurred (eg. the file is not currently
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_read(int bvfs_FD, void *buf, size_t count) {
}







/*
 * int bv_unlink(const char* fileName);
 *
 * This function is intended to delete a file that has been allocated within
 * the bvfs file system.
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to delete
 *             from the bvfs file system.
 *
 * Return Value
 *   int:  0 if the delete succeeded.
 *        -1 if some kind of failure occurred (eg. the file does not exist).
 *           Also, print a meaningful error to stderr prior to returning.
 */
int bv_unlink(const char* fileName) {
}







/*
 * void bv_ls();
 *
 * This function will list the contests of the single-directory file system.
 * First, you must print out a header that declares how many files live within
 * the file system. See the example below in which we print "2 Files" up top.
 * Then display the following information for each file listed:
 *   1) the file size in bytes
 *   2) the number of blocks occupied within bvfs
 *   3) the time and date of last modification (derived from unix timestamp)
 *   4) the name of the file.
 * An example of such output appears below:
 *    | 2 Files
 *    | bytes:  276, blocks: 1, Tue Nov 14 09:01:32 2017, bvfs.h
 *    | bytes: 1998, blocks: 4, Tue Nov 14 10:32:02 2017, notes.txt
 *
 * Hint: #include <time.h>
 * Hint: time_t now = time(NULL); // gets the current unix timestamp (32 bits)
 * Hint: printf("%s\n", ctime(&now));
 *
 * Input Parameters
 *   None
 *
 * Return Value
 *   void
 */
void bv_ls() {
  int numfiles = 0;
  for(int i = 0; i<256; i++){
    if(inode_arr[i].size!=0){
      numfiles++;
    }
  }
  printf("| %d files\n", numfiles);
  for(int i = 0; i<256; i++){
    if(inode_arr[i].size!=0){
      printf("| bytes: %d, blocks %d, %d, %s\n", inode_arr[i].size, inode_arr[i].size/512, inode_arr[i].time, inode_arr[i].fileName);
    }
  }
}
