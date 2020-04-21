#include <stdio.h>
#include <fcntl.h> // for open

#define BLOCK_SIZE 1024
#define INODE_SIZE 64

typedef	struct	{
  unsigned	short	isize;	//	2	bytes	- Blocks	for	i-list
  unsigned	short	fsize;	//	2	bytes	- Number	of	blocks
  unsigned	short	nfree;	//	2	bytes	- Pointer	of	free	block	array
  unsigned	short	ninode;	//	2	bytes	- Pointer	of	free	inodes	array
  unsigned	int	free[152];	//	Array	to	track	free	blocks
  unsigned	short	inode[200];	//	Array	to	store	free	inodes
  char	flock;
  char	ilock;
  unsigned	short	fmod;
  unsigned	short	time[2];	//	To	store	epoch
}	superblock_type;

typedef	struct{
  unsigned	short	flags;	//	Flag	of	a	file
  unsigned	short	nlinks;	//	Number	of	links	to	a	file
  unsigned	short	uid;	//	User	ID	of	owner
  unsigned	short	gid;	//	Group	ID	of	owner
  unsigned	int	size;	//	4	bytes	- Size	of	the	file
  unsigned	int	addr[11];	//	Block	numbers	of	the	file	location.	addr[10]	is	used	for	double	indirect block
  unsigned	short	actime[2];	//	Last	Access	time
  unsigned	short	modtime[2];	//	Last	modified	time
}	inode_type;

int main() {
  char v6Filename[256];
  char fileToRead[256];

  printf("Welcome. Enter the v6 filename: \n");
  scanf("%s", v6Filename);

  int fd = open(v6Filename, O_RDONLY);
  if (fd < 0) {
    printf("Error opening that file.\n");
    return 1;
  }

  printf("Enter the file you want to read from it: \n");
  scanf("%s", fileToRead);

  printf("you entered: %s, and: %s\n", v6Filename, fileToRead);
  return 0;
}