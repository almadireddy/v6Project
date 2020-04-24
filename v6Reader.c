#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include<errno.h>
extern int errno;

#include "types.h"

unsigned int blockToByteOffset(unsigned int blockNum) {
  return BLOCK_SIZE * (blockNum);
}

single_indirect_block_type getIndirectBlockFromFs(int fd, unsigned int
blockNum) {
  lseek(fd, blockToByteOffset(blockNum), SEEK_SET);
  single_indirect_block_type dir;
  read(fd, &dir, BLOCK_SIZE);

  return dir;
}

directory_block_type getDirectoryBlockFromFs(int fd, unsigned int blockNum) {
  lseek(fd, blockToByteOffset(blockNum), SEEK_SET);
  directory_block_type dir;
  read(fd, &dir, BLOCK_SIZE);

  return dir;
}

plain_block_type getPlainBlockFromFs(int fd, unsigned int blockNum) {
  lseek(fd, blockToByteOffset(blockNum), SEEK_SET);
  plain_block_type block;
  read(fd, &block, BLOCK_SIZE);

  return block;
}

unsigned int inodeToByteOffset(unsigned int inodeNumber) {
  // inodes start in third (index=2) block
  // inodeNumber input starts at 1. Access starts at 0.
  // inodeNumber = 1 => 2048 + (INODE_SIZE * 0)
  return 2048 + (INODE_SIZE * (inodeNumber - 1));
}

inode_type getInodeBlockFromFs(int fd, unsigned int inodeNumber) {
  lseek(fd, inodeToByteOffset(inodeNumber), SEEK_SET);
  inode_type inode;
  read(fd, &inode, INODE_SIZE);

  return inode;
}

// use inode size to determine how much of the block to read;
unsigned short recurseIntoFiles(int fd, char token[], unsigned int
currentInodeNumber) {
  char* nextTok = strtok(NULL, "/");

  printf("Looking for: %s\n", token);

  inode_type currentInode = getInodeBlockFromFs(fd, currentInodeNumber);

  unsigned short isDir = 0b0100000000000000;

  if (currentInode.flags & isDir) {
    for (int j = 0; j < INODE_ADDR_LENGTH; ++j) {
      unsigned int blockNumber = currentInode.addr[j];

      if (blockNumber == 0) {
        continue;
      }

      directory_block_type currentDir = getDirectoryBlockFromFs(fd,
          blockNumber);

      for (int i = 0; i < ENTRIES_IN_DIR; ++i) {
        unsigned short iNumber = currentDir.entries[i].iNumber;

        if (iNumber == 0 || iNumber == currentInodeNumber) {
          continue;
        }

        int same = strcmp(token, currentDir.entries[i].fileName);
        if (same == 0) {
          if (nextTok == NULL) {
            return iNumber;
          }
          return recurseIntoFiles(fd, nextTok, iNumber);
        }
      }
    }
  }
  return 0;
}

void findFile(int fd, char path[]) {
  char * firstToken = strtok(path, "/");
  // traverse directories and return the inode for file we are looking for.
  unsigned short iNumberForFile = recurseIntoFiles(fd, firstToken, 1);

  if (iNumberForFile == 0) {
    printf("Could not find that file.\n");
    return;
  }

  inode_type inodeForFile = getInodeBlockFromFs(fd, iNumberForFile);

  unsigned short regularFile = 0b1000000000000000;
  unsigned short sizeCheck = 0b0001000000000000;
  if (inodeForFile.flags & regularFile) {
    printf("Found file, attempting to retrieve and write to myoutputfile.txt\n");

    int writefile = open("myoutputfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (writefile < 0) {
      printf("There was an error opening output file. Error Number %d\n",
             errno);
    }

    unsigned int sizeToWrite  = inodeForFile.size;

    if (inodeForFile.flags & sizeCheck) {
      // every block here is a list of block numbers.
      // each one has 256 ints that point to data blocks.
      for (int i = 0; i < INODE_ADDR_LENGTH - 1; ++i) {
        single_indirect_block_type block = getIndirectBlockFromFs(fd,
            inodeForFile.addr[i]);

        for (int j = 0; j < ADDRS_IN_INDIRECT_BLOCK; ++j) {
          plain_block_type b = getPlainBlockFromFs(fd, block.addrs[j]);
          unsigned int toWrite;
          if (BLOCK_SIZE < sizeToWrite) {
            toWrite = BLOCK_SIZE;
          } else {
            toWrite = sizeToWrite;
          }
          int writeStatus = write(writefile, &b.text, toWrite);
          if (writeStatus < 0) {
            printf("There was an error writing to myoutputfile.txt.\n");
          }
          sizeToWrite -= writeStatus;
        }
      }
    } else {
      for (int i = 0; i < INODE_ADDR_LENGTH; ++i) {
        unsigned int dataBlockNum = inodeForFile.addr[i];

        if (dataBlockNum != 0) {
          plain_block_type b = getPlainBlockFromFs(fd, inodeForFile.addr[i]);
          unsigned int toWrite;
          if (BLOCK_SIZE < sizeToWrite) {
            toWrite = BLOCK_SIZE;
          } else {
            toWrite = sizeToWrite;
          }
          int writeStatus = write(writefile, &b.text, toWrite);
          if (writeStatus < 0) {
            printf("There was an error writing to myoutputfile.txt.\n");
          }
          sizeToWrite -= writeStatus;
        }
      }
    }
    int closeStatus = close(writefile);
    if (closeStatus < 0) {
      printf("There was an error closing myoutputfile.txt.\n");
    }
    return;
  }
}

int main() {
  char v6Filename[256];
  char fileToRead[256];

  printf("Welcome. Enter the v6 filename: \n");
  scanf("%s", v6Filename);

  int fd = open(v6Filename, 2);
  if (fd < 0) {
    printf("Error opening that file.\n");
    return 1;
  }

  printf("Enter the file you want to read from it: \n");
  scanf("%s", fileToRead);
  printf("Looking for that file. Running findFile().\n");

  // start off at super block (block 2)
  // by going 1024 bytes from the left.
  lseek(fd, 1024, SEEK_SET);
  superblock_type superBlock;
  read(fd, &superBlock, BLOCK_SIZE);

  findFile(fd, fileToRead);

  return 0;
}
