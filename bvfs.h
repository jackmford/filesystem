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
#include <time.h>
#include <math.h>

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
  time_t timeinfo; // 8 bytes
  short address[128]; // 256 bytes
  int read_cursor;
  int is_open;
  int dummydata[49];
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
short SBLOCK_ARRAY_ID;
short superblock_array[256];
struct iNode inode_arr[256];
const int ENDOFMETA = sizeof(inode_arr)+512;
short superblock_ids[64];
short read_only_files[256];


short get_new_address();
void give_back_block(short block_address);

// Write inode array to file
void write_inode(){
  lseek(GLOBAL_PFD, INODE_START, SEEK_SET);
  write(GLOBAL_PFD, (void*)(&inode_arr), sizeof(inode_arr));
}

// Write superblock to file
void write_superblock(){
  lseek(GLOBAL_PFD, SBLOCK_ARRAY_ID*BLOCK_SIZE, SEEK_SET);
  write(GLOBAL_PFD, (void*)&superblock_array, sizeof(superblock_array));
}

// Write all linked list nodes to file
void write_superblock_ids(){
  lseek(GLOBAL_PFD, 4, SEEK_SET);
  write(GLOBAL_PFD, (void*)&superblock_ids, sizeof(superblock_ids));

}

// Reads in a linked list node
void set_address_block() {
  // Presume the addresses are all null
  // Seek to the one this address block points to
  lseek(GLOBAL_PFD, superblock_array[255]*BLOCK_SIZE, SEEK_SET);
  SBLOCK_ARRAY_ID = superblock_array[255];

  // Reload the in memory address block
  short temp;
  for(short i = 0; i < MAX_FILES; i++) {
    read(GLOBAL_PFD, &temp, 2);
    superblock_array[i] = temp;
  }
  write_superblock();
}

// Give block back if not being used anymore (all empty or unlinking)
// Finds a place in the superblock array for it to live
void give_back_block(short block_address){
  // Look through superblock_array and see if there is an open space currently
  int open = 0;
  int offset = 0;
  int ctr = 0;
  //Check if this is a superblock id, if it is, remove it from superblock_id array in memory
  for(int i=0; i<64; i++){
    if(superblock_ids[i] == block_address){
      superblock_ids[i] = 0;
    }
  }
  // While you havent found a place to put the block
  while(open != 1){
    for(int i = 0; i < MAX_FILES; i++){
      if(superblock_array[i] == 0){
        open = 1;
        offset = i;
        break;
      }
      ctr++;
      // If were at the end of a node in linked list, get a new node and continue looking
      if(ctr == 255 && open == 0){
        set_address_block();
      }
    }
  }
  // Once a spot has been found, add it to location in linked list node
  if(open == 1){
    superblock_array[offset] = block_address;
    write_superblock();
  }
  else if(open == 0){
    // Didn't find any superblock space 
    // Look to create a new linked list node if there is empty room 
    for(int i = 0; i<64; i++){
      if(superblock_ids[i] == 0){
        // Finding an address for new node and adding this address to that linked list node
        short return_addr = 0;
        short i = 0;
        short addr = get_new_address();
        superblock_ids[i] = addr;
        short newaddrs[MAX_FILES];
        newaddrs[0] = block_address;
        lseek(GLOBAL_PFD, addr*BLOCK_SIZE, SEEK_SET);
        write(GLOBAL_PFD, (void*)&newaddrs, sizeof(newaddrs));
        write_superblock_ids();
        return;
      }
    }
  }
  // Completely full
  else{
    char err[] = "Bad error there are no spaces to put an address\n";
    write(2, &err, sizeof(err));
  }
}

// Get a new memory address
short get_new_address() {
  // Try to find an open block address
  // to hand back
  short return_addr = 0;
  short i = 0;
  while (return_addr == 0) {
    // If i is 255, we have not found a new address
    // Call function to find new block of addresses
    // Reset i to start looking again
    if (i == 255) {
      // Give up super block and put into system
      for(int i = 0; i<64; i++){
        if(superblock_ids[i] == SBLOCK_ARRAY_ID){
          // Found the empty superblock node
          // Relingquish to the system
          give_back_block(SBLOCK_ARRAY_ID);
          superblock_ids[i] = 0;
        }
      }
      // Get new node from linked list and continue looking for address
      set_address_block();
      i = 0;
    }
    // If found an open address in the linked list node,
    // Return it and set it to be in use
    if (superblock_array[i] != 0){
      return_addr = superblock_array[i];
      superblock_array[i] = 0;
      break;
    }
    i++;
  }
  return return_addr;
}

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
  // Try to open file
  int pFD = open(fs_fileName, O_CREAT | O_RDWR | O_EXCL, 0644);
  // If file descriptor error
  if (pFD < 0) {
    if (errno == EEXIST) {
      // file already exists
      pFD = open(fs_fileName, O_CREAT | O_RDWR , S_IRUSR | S_IWUSR);
      GLOBAL_PFD = pFD;
      INIT_FLAG = 1; // Assert we have ran init

      // Get the super block position
      lseek(pFD, 0, SEEK_SET);
      int sblock_start;
      read(pFD, &sblock_start, 4);

      // Initialize superblock array 
      short temp = 0;
      int ctr = 0;
      short sarr[MAX_FILES];

      // If super block contains all null addresses, 
      // move to another super block and test again
      // until a super block with an open data block address
      // is found
      //
      // This becomes our current in memory node from the linked list to grab addresses from
      while(temp == 0) {
        lseek(pFD, sblock_start, SEEK_SET);
        for (short i = 0; i < MAX_FILES; i++) {
          read(pFD, &temp, 2);
          sarr[ctr] = temp;
          ctr++;
          if (temp != 0) // Asserts one usable data block
            break;
        }
        if(temp == 0)
          sblock_start = sarr[255] * BLOCK_SIZE;
      }

      // Read in the addresses in that superblock to memory
      lseek(pFD, sblock_start, SEEK_SET);
      for(short i = 0; i < MAX_FILES; i++) {
        read(pFD, &temp, 2);
        superblock_array[i] = temp;
      }

      // Location of the current node in the linked list
      SBLOCK_ARRAY_ID = sblock_start;

      // Read in inode array from file.
      lseek(pFD, INODE_START, SEEK_SET);
      read(pFD, &inode_arr, sizeof(inode_arr));

      // Reading in IDs of the nodes of the linked list from partition
      short blockidarray[63];
      lseek(pFD, 4, SEEK_SET);
      read(pFD, &blockidarray, sizeof(blockidarray));
      for(int i = 0; i<63; i++){
        superblock_ids[i] = blockidarray[i];
      }
      return 0;
    }
    else {
      // Something bad must have happened... check errno?
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

    // Get block num and write it to file
    time_t rawtime;
    rawtime = time(NULL);

    struct iNode test;
    lseek(pFD, 0, SEEK_SET); // Seek to 0
    write(pFD, (void*)(&ENDOFMETA), 4); // Write the start of the super block linked list

    // Seek to beginning of iNodes (end of superblock)
    lseek(pFD, INODE_START, SEEK_SET);
    write(pFD, (void*)(&inode_arr), sizeof(inode_arr));

    // Go to start of linked list and piece it together
    lseek(pFD, ENDOFMETA, SEEK_SET);
    // Write addresses to next 256 blocks
    short blockNum = (ENDOFMETA/BLOCK_SIZE);

    // for all super block ids
    short blocklist[63];
    int ctr = 0;

    // Write linked list
    SBLOCK_ARRAY_ID = blockNum;
    while (blockNum < MAX_BLOCKS) {
      for (short i = blockNum+1; i<=(blockNum+MAX_FILES);i++) {
        if(i*BLOCK_SIZE<=PARTITION_SIZE)
          write(pFD, (void*)&i, 2);
        else
          write(pFD, 0, 2);
      }
      blocklist[ctr] = blockNum;
      ctr++;

      blockNum += MAX_FILES;
      lseek(pFD, blockNum * BLOCK_SIZE, SEEK_SET);
    }

    // Load first linked list node into memory to use
    lseek(pFD, ENDOFMETA, SEEK_SET);
    for(int i = 0; i<MAX_FILES; i++){
      short tmp;
      read(pFD, &tmp, 2);
      superblock_array[i] = tmp;
    }

    // write all linked list node ids to front superblock
    lseek(pFD, 4, SEEK_SET);
    write(pFD, (void*)&blocklist, sizeof(blocklist));

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
    // Write all essential data structures back to file
    //
    // Reset files cursor
    for(int i = 0; i<MAX_FILES; i++){
      if(inode_arr[i].timeinfo!=0)
        inode_arr[i].read_cursor = 0;
    }
    write_inode();
    write_superblock();
    write_superblock_ids();
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
  // Find null byte in fileName
  int foundNull = 0;
  for (int i=0; i<strlen(fileName)+1; i++) {
    if(fileName[i] == '\0'){
      foundNull = 1;
      break;
    }
  }
  // If nonexistent, return -1
  if (foundNull != 1) {
    char err[] = "FileName not nullbyte ended.\n";
    write(2, &err, sizeof(err));
    return -1;
  }

  // Filename over 32 bytes
  int name_length = sizeof(fileName);
  if(strlen(fileName)>=32){
    char err[] = "FileName too long.\n";
    write(2, &err, sizeof(err));
    return -1;
  }

  // See if the file exists, found_flag will be 1 if it does
  int found_flag = 0;
  int file_index = -1;
  for(int i = 0; i<MAX_FILES; i++){
    if(strcmp(inode_arr[i].fileName, fileName)  == 0 ){
      found_flag = 1;
      file_index = i;
      break;
    }
  }

  // No file found and opened in read mode
  if(found_flag == 0 && mode == 0){
    char err[] = "Opened in read mode but no file found.\n";
    write(2, &err, sizeof(err));
    return -1;
  }

  // Look through inode array for free spot
  int free_inode_index = -1;
  if(found_flag == 0){
    for(int i = 0; i<MAX_FILES; i++){
      if(inode_arr[i].timeinfo == 0){
        free_inode_index = i;
        break;
      }
    }
  }

  // Opening brand new file
  if(found_flag == 0 && mode != 0){
    for(int j=0; j<MAX_FILES; j++){
      // Found address in current superblock to use for the file
      // Initialize the inode values and return its first address
      if(superblock_array[j] != 0){
        time_t newtime = time(NULL);
        char name[32];
        struct iNode tmp;
        strncpy(tmp.fileName, fileName, 31);
        tmp.fileName[31] = 0;
        tmp.address[0] = superblock_array[j];
        for (int k = 1; k<MAX_FILE_BLOCKS;k++)
          tmp.address[k] = 0;
        tmp.size = 0;
        tmp.timeinfo = newtime;
        tmp.read_cursor = 0;
        tmp.is_open = 1;
        inode_arr[free_inode_index] = tmp;
        superblock_array[j] = 0;
        return inode_arr[free_inode_index].address[0];
      }
    }
  }
  else if(found_flag == 1){
    if(mode == 2){
      // Truncate, return first address after giving others back to system for use
      for(int i = 1; i<(inode_arr[file_index].size/BLOCK_SIZE+1)/BLOCK_SIZE; i++){
        give_back_block(inode_arr[file_index].address[i]);
        inode_arr[file_index].address[i] = 0;
      }
      // Reset size to 0, mark file as open, reset cursor
      inode_arr[file_index].size = 0;
      inode_arr[file_index].is_open = 1;
      inode_arr[file_index].read_cursor = 0;
      return inode_arr[file_index].address[0];
    }
    else if(mode == 1){
      // Concat
      inode_arr[file_index].is_open = 1;
      return inode_arr[file_index].address[0];
    }
    else if(mode == 0){
      // Read, add to read only files list
      for(int i = 0; i<MAX_FILES; i++){
        if(read_only_files[i] == 0){
          read_only_files[i] = inode_arr[file_index].address[0];
          break;
        }
      }
      inode_arr[file_index].is_open = 1;
      inode_arr[file_index].read_cursor = 0;
      return inode_arr[file_index].address[0];
    }
  }
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
  int check = 0;
  int inode_index = 0;
  // Find the file, if it isnt open, error out
  for(int i = 0; i < MAX_FILES; i++){
    for(int j = 0; j<MAX_FILE_BLOCKS; j++){
      if(bvfs_FD == inode_arr[i].address[0]) {
        // If file not open, don't return anything
        if (inode_arr[i].is_open == 0) {
          char err[] = "File not open.\n";
          write(2, &err, sizeof(err));
          return -1;
        }
        check = 1;
        inode_index = i;
        break;
      }
    }
  }
  if(check == 1){
    for(int i = 0; i<MAX_FILES; i++){
      if(read_only_files[i] == inode_arr[inode_index].address[0]){
        read_only_files[i] = 0;
        break;
      }
    }
    // Write this file's inode back to file to save
    struct iNode tmp = inode_arr[inode_index];
    tmp.is_open = 0; // Set file to 'closed'
    short offset = inode_index*BLOCK_SIZE+BLOCK_SIZE;
    lseek(GLOBAL_PFD, offset, SEEK_SET);
    write(GLOBAL_PFD, (void*)&tmp, sizeof(tmp));
    return 0;
  }
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
  // Try to find bvfs_FD in inode array
  short inode_index;
  short found = -1;
  int numblocks = (count + BLOCK_SIZE -1)/BLOCK_SIZE;
  short new_addrs[MAX_FILE_BLOCKS];
  new_addrs[0] = bvfs_FD;

  // Try to find file
  for(int i = 0; i<MAX_FILES; i++){
    if (inode_arr[i].address[0] == read_only_files[i] && inode_arr[i].address[0]==bvfs_FD) {
      char err[] = "File in read only.\n";
      write(2, &err, sizeof(err));
      return -1;
    }
    // If the file descriptor is found
    if(inode_arr[i].address[0]==bvfs_FD){
      // Is the file open, error out if not
      if (inode_arr[i].is_open != 1) {
        char err[] = "File not open.\n";
        write(2, &err, sizeof(err));
        return -1;
      }
      inode_index = i;
      // If the cursor is 0 (read/truncate mode), set it and seek
      if (inode_arr[i].read_cursor == 0) {
        inode_arr[i].read_cursor = bvfs_FD*BLOCK_SIZE;
        lseek(GLOBAL_PFD, inode_arr[i].read_cursor, SEEK_SET);
      }

      found = 1;
      break;
    }
  }

  if (found != 1){
    char err[] = "File not found\n";
    write(2, &err, sizeof(err));
    return -1;
  }

  int bytes_left = count;
  int total = 0;
  int new_addrs_loc = 0;
  int new_blocks = 0;
  /* 
     While there are bytes to write
     write to end of block on
     if need a new one
     get one, save it
     move to that block
     repeat
     */
  while (bytes_left > 0) {
    // Cursor is somewhere in middle of a block
    // Write the missing chunk to fill it or the amount of bytes left
    if (inode_arr[inode_index].read_cursor % BLOCK_SIZE != 0) {
      lseek(GLOBAL_PFD, inode_arr[inode_index].read_cursor, SEEK_SET);
      int writing_bytes = (inode_arr[inode_index].read_cursor % BLOCK_SIZE);
      int tmp;
      int minbytes;
      if(bytes_left < BLOCK_SIZE-writing_bytes){
        tmp = write(GLOBAL_PFD, buf, bytes_left); 
        minbytes = bytes_left;
      }
      else{
        tmp = write(GLOBAL_PFD, buf, BLOCK_SIZE-writing_bytes); 
        minbytes = BLOCK_SIZE-writing_bytes;
      }
      total += tmp;
      bytes_left -= minbytes;
      buf+=minbytes;
      if(bytes_left <= 0){
        inode_arr[inode_index].read_cursor += tmp;
        break;
      }
    }
    // There is an entire block or more left to write
    else if (bytes_left > BLOCK_SIZE) {
      int tmp = write(GLOBAL_PFD, buf, BLOCK_SIZE);
      total += tmp;
      bytes_left -= BLOCK_SIZE;
      buf+=BLOCK_SIZE;
      if(bytes_left <= 0){
        inode_arr[inode_index].read_cursor += tmp;
        break;
      }
    }
    // There is less than 512 bytes to write
    else if (bytes_left <= BLOCK_SIZE) {
      lseek(GLOBAL_PFD, inode_arr[inode_index].read_cursor, SEEK_SET);
      int tmp = write(GLOBAL_PFD, buf, bytes_left);
      total += tmp;
      buf+=tmp;
      bytes_left -= tmp;
      if(bytes_left <= 0){
        inode_arr[inode_index].read_cursor += tmp;
        break;
      }
    }

    // If there are still bytes to write
    // Get new address, 
    // Add it to new address list,
    // Move the cursor, 
    // Seek
    if (bytes_left > 0) {
      short new_addr = get_new_address();
      new_addrs[new_addrs_loc] = new_addr;
      new_addrs_loc++;
      new_blocks++;
      inode_arr[inode_index].read_cursor = new_addr * BLOCK_SIZE;
      lseek(GLOBAL_PFD, inode_arr[inode_index].read_cursor, SEEK_SET);
    }
    else{
      break;
    }
  }

  // Make changes to inode
  inode_arr[inode_index].timeinfo = time(NULL);
  inode_arr[inode_index].size += total;
  short last_place = 0;
  // If we are on truncate, inode gets all new addresses
  if (inode_arr[inode_index].address[0] != bvfs_FD && inode_arr[inode_index].address[1] == 0) {
    memcpy(inode_arr[inode_index].address, new_addrs, sizeof(new_addrs));
    last_place = 0;
  }
  // If we are on concat
  else {
    // Find the last address we need have in addresses
    for (short i = 0; i < 127; i++) {
      if (inode_arr[inode_index].address[i] != 0 && inode_arr[inode_index].address[i+1] == 0) {
        last_place = i+1;
        if(inode_arr[inode_index].size<BLOCK_SIZE)
          last_place = 0;
        break;
      }
    }
    // Starting at the last used address,
    // Append on all of the new addresses that we used as well
    short t = 0;
    for (short i = last_place; i < new_blocks+last_place; i++) {
      inode_arr[inode_index].address[i] = new_addrs[t];
      t++;
    }
  }
  return total;
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
  // Try to find bvfs_FD in inode array
  short inode_index = -1;
  int found = -1;
  int read_found = 0;
  for(int i = 0; i<MAX_FILES; i++){
    if(bvfs_FD == read_only_files[i]){
      read_found = 1;
    }
  }

  for(int i = 0; i<MAX_FILES; i++){
    // If the file has been found and it has the File Descriptor in the inode
    if(read_found == 1 && inode_arr[i].address[0]==bvfs_FD){
      found = 1;
      inode_index = i;
      // Set the cursor to appropriate read place
      if (inode_arr[i].read_cursor == 0)
        inode_arr[i].read_cursor = bvfs_FD*BLOCK_SIZE;
      // If cursor is at end of file, can no longer read -- file must be closed
      if (inode_arr[i].read_cursor == inode_arr[i].address[0]*BLOCK_SIZE+inode_arr[i].size) {
        char err[] = "EOF error.\n";
        write(2, &err, sizeof(err));
        return -1;
      }
      break;
    }
  }

  if(found < 0){
    char err[] = "File not found.\n";
    write(2, &err, sizeof(err));
    return -1;
  }


  int bytes_left = count;
  int total = 0;
  int blocks_to_read = (count + BLOCK_SIZE -1)/BLOCK_SIZE;

  int ctr = 1;
  // While there are bytes to read
  // Keep track of cursor, total bytes read, and increment buf as we read
  while(bytes_left > 0){
    lseek(GLOBAL_PFD, inode_arr[inode_index].read_cursor, SEEK_SET);
    // Read remaining bytes
    if(bytes_left <= BLOCK_SIZE){
      total += read(GLOBAL_PFD, buf, bytes_left); 
      buf+=bytes_left;
      inode_arr[inode_index].read_cursor += bytes_left;
      return total;
    }
    else{
      // Read bytes
      int tmp = read(GLOBAL_PFD, buf, BLOCK_SIZE-(inode_arr[inode_index].read_cursor % BLOCK_SIZE)); 
      total += tmp;
      buf+=BLOCK_SIZE-(inode_arr[inode_index].read_cursor % BLOCK_SIZE);
      inode_arr[inode_index].read_cursor += tmp;;
      bytes_left -= tmp;
      ctr++;
    }
  }
  return total;
}

/*
 * int bv_unnlink(const char* fileName);
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
  //Remove fileName from inode array
  //Give back all blocks to the system to reuse
  int check = 0;
  int inode_index = 0;
  for(int i = 0; i < MAX_FILES; i++){
    if(strcmp(inode_arr[i].fileName, fileName) == 0){
      check = 1;
      inode_index = i;
    }
  }
  if(check == 0){
    char err[] = "File not found.\n";
    write(2, &err, sizeof(err));
    return -1;
  }
  else{
    // Give back blocks to the system
    int c = (inode_arr[inode_index].size+BLOCK_SIZE-1)/BLOCK_SIZE;
    for(int i = 0; i<(inode_arr[inode_index].size+BLOCK_SIZE-1)/BLOCK_SIZE; i++){
      give_back_block(inode_arr[inode_index].address[i]);
    }
    // If it was in read only mode, remove it from that list
    for(int i = 0; i<MAX_FILES; i++){
      if(read_only_files[i] == inode_arr[inode_index].address[0])
        read_only_files[i] = 0;
      break;
    }
    // Write an empty inode to its place and write the whole inode list to system
    struct iNode tmp = {0, 0, 0, 0, 0, 0};
    inode_arr[inode_index] = tmp;
    write_inode();
    return 0;
  }
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
  // Iterate through iNode array to retrieve information.
  // Find number of files in the inode array
  int numfiles = 0;
  for(int i = 0; i<MAX_FILES; i++){
    if(inode_arr[i].timeinfo!=0){
      numfiles++;
    }
  }
  // Print information from the inode
  printf("%d Files\n", numfiles);
  for(int i = 0; i<MAX_FILES; i++){
    int ceil = (inode_arr[i].size + BLOCK_SIZE -1)/BLOCK_SIZE;
    if(inode_arr[i].timeinfo!=0){
      printf("| bytes: %d blocks: %d, %.24s, %s\n", inode_arr[i].size, ceil, ctime(&inode_arr[i].timeinfo), inode_arr[i].fileName);
    }
  }
}
