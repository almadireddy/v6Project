#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

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

typedef	struct{
  unsigned	short	flags;	//	Flag	of	a	file
  unsigned	short	nlinks;	//	Number	of	links	to	a	file
  unsigned	short	uid;	//	User	ID	of	owner
  unsigned	short	gid;	//	Group	ID	of	owner
  unsigned	int	size;	//	4	bytes	- Size	of	the	file
  unsigned	int	addr[11];	//	Block	numbers	of	the	file	location.
  // addr[10] is	used for	double	indirect block
  unsigned	short	actime[2];	//	Last	Access	time
  unsigned	short	modtime[2];	//	Last	modified	time
}	inode_type;

// directory entry is 16 bytes.
// bytes 0 and 1 together are the i-number
// bytes 2-15 are the file name (14 characters)
typedef struct {
  unsigned short iNumber;
  char fileName[14];
} directory_entry_type;

typedef struct {
  directory_entry_type entries[64];
} directory_block_type;

unsigned int blockToByteOffset(unsigned int blockNum) {
  return BLOCK_SIZE * (blockNum);
}

unsigned int inodeToByteOffset(unsigned int inodeNumber) {
  // inodes start in third (index=2) block
  // inodeNumber input starts at 1. Access starts at 0.
  // inodeNumber = 1 => 2048 + (INODE_SIZE * 0)
  return 2048 + (INODE_SIZE * (inodeNumber - 1));
}

int main() {
  char v6Filename[256];
  char fileToRead[256];

  printf("Welcome. Enter the v6 filename: \n");
  scanf("%s", v6Filename);

  int fd = open("v6fs", 2);
  if (fd < 0) {
    printf("Error opening that file.\n");
    return 1;
  }

//  printf("Enter the file you want to read from it: \n");
//  scanf("%s", fileToRead);
//  printf("Looking for that file.\n");

  // start off at super block (block 2)
  // by going 1024 bytes from the left.
  lseek(fd, 1024, SEEK_SET);

  superblock_type superBlock;
  read(fd, &superBlock, BLOCK_SIZE);

  inode_type firstInode;
  lseek(fd, inodeToByteOffset(1), SEEK_SET);
  read(fd, &firstInode, INODE_SIZE);

  unsigned int rootAddr = firstInode.addr[0];
  unsigned int blockNum = blockToByteOffset(rootAddr);

  lseek(fd, blockNum, SEEK_SET);

  directory_block_type rootDir;
  read(fd, &rootDir, BLOCK_SIZE);

  inode_type dir2Inode;
  lseek(fd, inodeToByteOffset(4), SEEK_SET);
  read(fd, &dir2Inode, INODE_SIZE);

  unsigned int dir2BlockNum = blockToByteOffset(dir2Inode.addr[0]);
  directory_block_type dir2;
  lseek(fd, dir2BlockNum, SEEK_SET);
  read(fd, &dir2, BLOCK_SIZE);
  return 0;
}
