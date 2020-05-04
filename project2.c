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

indirect_block_type getIndirectBlockFromFs(int fd, unsigned int
blockNum) {
  lseek(fd, blockToByteOffset(blockNum), SEEK_SET);
  indirect_block_type dir;
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

unsigned short lookThroughDirectory(int fd, directory_block_type *currentDir, unsigned int currentInodeNumber, char token[], char nextTok[]) {
  for (int i = 0; i < ENTRIES_IN_DIR; ++i) {
    directory_entry_type currentEntry = currentDir->entries[i];
    unsigned short iNumber = currentEntry.iNumber;

    if (iNumber == 0 || iNumber == currentInodeNumber || iNumber > superBlock.isize*INODE_SIZE) {
      continue;
    }

    int same = strcmp(token, currentEntry.fileName);
    if (same == 0) {
      if (nextTok == NULL) {
        return iNumber;
      }
      return recurseIntoFiles(fd, nextTok, iNumber);
    }
  }
  return 0;
}

// use inode size to determine how much of the block to read;
unsigned short recurseIntoFiles(int fd, char token[], unsigned int currentInodeNumber) {
  char* nextTok = strtok(NULL, "/");

  printf("Looking for: %s\n", token);

  inode_type currentInode = getInodeBlockFromFs(fd, currentInodeNumber);

  if (currentInode.flags & dirFlag) {
    if (currentInode.flags & largeFileFlag) {
      // large file directory
      for (int i = 0; i < INODE_ADDR_LENGTH - 1; ++i) {
        unsigned int indirectBlockNum = currentInode.addr[i];
        if (indirectBlockNum == 0) {
          continue;
        }

        indirect_block_type indirectBlock = getIndirectBlockFromFs(fd, indirectBlockNum);

        for (int j = 0; j < ADDRS_IN_INDIRECT_BLOCK; ++j) {
          unsigned int currentAddress = indirectBlock.addrs[j];

          if (currentAddress == 0) {
            continue;
          }

          directory_block_type currentDir = getDirectoryBlockFromFs(fd, currentAddress);

          return lookThroughDirectory(fd, &currentDir, currentInodeNumber, token, nextTok);
        }
      }
    }
    else {
      // small file directory
      for (int j = 0; j < INODE_ADDR_LENGTH - 1; ++j) {
        unsigned int blockNumber = currentInode.addr[j];

        if (blockNumber == 0) {
          continue;
        }

        directory_block_type currentDir = getDirectoryBlockFromFs(fd, blockNumber);

        return lookThroughDirectory(fd, &currentDir, currentInodeNumber, token, nextTok);
      }
    }
  }
  return 0;
}

int writeToFile(int fsFd, int writeFd, unsigned int dataBlockNum, unsigned int* sizeToWrite) {
  if (dataBlockNum != 0) {
    plain_block_type b = getPlainBlockFromFs(fsFd, dataBlockNum);
    unsigned int toWrite;
    if (BLOCK_SIZE < *sizeToWrite) {
      toWrite = BLOCK_SIZE;
    } else {
      toWrite = *sizeToWrite;
    }
    int writeStatus = write(writeFd, &b.text, toWrite);
    if (writeStatus < 0) {
      printf("There was an error writing to myoutputfile.txt.\n");
      return -1;
    }
    *sizeToWrite -= writeStatus;
  }
  return 0;
}

void findFile(int fd, char path[]) {
  lseek(fd, 2048, SEEK_SET);
  char * firstToken = strtok(path, "/");
  // traverse directories and return the inode for file we are looking for.
  unsigned short iNumberForFile = recurseIntoFiles(fd, firstToken, 1);

  if (iNumberForFile == 0) {
    printf("Could not find that file.\n");
    return;
  }

  inode_type inodeForFile = getInodeBlockFromFs(fd, iNumberForFile);

  if (inodeForFile.flags & regularFileFlag) {
    printf("Found file, attempting to retrieve and write to myoutputfile.txt\n");

    int writefile = open("myoutputfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (writefile < 0) {
      printf("There was an error opening output file. Error Number %d\n", errno);
      return;
    }

    unsigned int sizeToWrite = inodeForFile.size;

    if (inodeForFile.flags & largeFileFlag) {
      for (int i = 0; i < INODE_ADDR_LENGTH - 1; ++i) {
        indirect_block_type block = getIndirectBlockFromFs(fd, inodeForFile.addr[i]);

        for (int j = 0; j < ADDRS_IN_INDIRECT_BLOCK; ++j) {
          unsigned int indirectBlockNum = block.addrs[j];
          if (indirectBlockNum == 0) continue;

          int s = writeToFile(fd, writefile, indirectBlockNum, &sizeToWrite);
          if (s < 0) return;
        }
      }
      unsigned int lastAddr = inodeForFile.addr[INODE_ADDR_LENGTH - 1];
      // check last addr int to handle triple indirect
      if (lastAddr > 0) {
        indirect_block_type tripleIndirectBlock = getIndirectBlockFromFs(fd, lastAddr);

        for (int i = 0; i < ADDRS_IN_INDIRECT_BLOCK; ++i) {
          unsigned int doubleIndirectBlockNumber = tripleIndirectBlock.addrs[i];
          if (doubleIndirectBlockNumber == 0) continue;

          indirect_block_type doubleIndirectBlock = getIndirectBlockFromFs(fd, doubleIndirectBlockNumber);

          for (int j = 0; j < ADDRS_IN_INDIRECT_BLOCK; ++j) {
            unsigned int singleIndirectBlockNumber = doubleIndirectBlock.addrs[j];
            if (singleIndirectBlockNumber == 0) continue;

            indirect_block_type singleIndirectBlock = getIndirectBlockFromFs(fd, singleIndirectBlockNumber);

            for (int k = 0; k < ADDRS_IN_INDIRECT_BLOCK; ++k) {
              unsigned int blockNumber = singleIndirectBlock.addrs[k];
              if (blockNumber == 0) continue;

              int s = writeToFile(fd, writefile, singleIndirectBlockNumber, &sizeToWrite);
              if (s < 0) return;
            }
          }
        }
      }
    } else {
      for (int i = 0; i < INODE_ADDR_LENGTH - 1; ++i) {
        int s = writeToFile(fd, writefile, inodeForFile.addr[i], &sizeToWrite);
        if (s < 0) return;
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
  char fileToRead[512];

  printf("Welcome. Enter the v6 filename: \n");
  scanf("%s", v6Filename);

  int fd = open(v6Filename, 2);
  if (fd < 0) {
    printf("Error opening that file.\n");
    return 1;
  }

  printf("Enter the file you want to read from it (leading slash is optional): "
         "\n");
  scanf("%s", fileToRead);
  printf("Looking for that file. Running findFile().\n");

  // start off at super block (block 2)
  // by going 1024 bytes from the left.
  lseek(fd, 1024, SEEK_SET);
  read(fd, &superBlock, BLOCK_SIZE);

  findFile(fd, fileToRead);

  return 0;
}
