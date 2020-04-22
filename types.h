//
// Created by Aahlad Madireddy on 4/22/20.
//

#ifndef FS_TYPES_H
#define FS_TYPES_H


#define BLOCK_SIZE 1024
#define INODE_SIZE 64

typedef	struct {
  unsigned short	isize;	//	2	bytes	- Blocks	for	i-list
  unsigned short	fsize;	//	2	bytes	- Number	of	blocks
  unsigned short	nfree;	//	2	bytes	- Pointer	of	free	block	array
  unsigned short ninode; //	2	bytes	- Pointer	of	free	inodes	array
  unsigned int free[152]; // Array to track free blocks
  unsigned short inode[200]; // Array to store free inodes
  char flock;
  char ilock;
  unsigned short fmod;
  unsigned short time[2]; // To store	epoch
}	superblock_type;

#define INODE_ADDR_LENGTH 11
typedef	struct{
  unsigned short flags; // Flag of a file
  unsigned short nlinks; // Number of links to a file
  unsigned short uid; // User ID of owner
  unsigned short gid; // Group ID of owner
  unsigned int size; // 4 bytes - Size of the file
  unsigned int addr[INODE_ADDR_LENGTH]; // Block numbers of the file location.
  // addr[10] is used for double indirect block
  unsigned short actime[2]; // Last Access time
  unsigned short modtime[2]; // Last modified time
} inode_type;

// directory entry is 16 bytes.
// bytes 0 and 1 together are the i-number
// bytes 2-15 are the file name (14 characters)
typedef struct {
  unsigned short iNumber;
  char fileName[14];
} directory_entry_type;

#define ENTRIES_IN_DIR 64
typedef struct {
  directory_entry_type entries[ENTRIES_IN_DIR];
} directory_block_type;

typedef struct {
  char text[BLOCK_SIZE];
} plain_block_type;

#endif //FS_TYPES_H
