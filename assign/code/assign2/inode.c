#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

#define INODE_SIZE 32  // size in bytes
#define INODE_PER_BLOCK (DISKIMG_SECTOR_SIZE/INODE_SIZE)
#define BLOCKS_PER_INDIR 256
#define NUM_INDIR_BLOCKS 7
#define TOTAL_BLOCKS_FROM_INDIR (BLOCKS_PER_INDIR * NUM_INDIR_BLOCKS)

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {

}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {

}

int inode_getsize(struct inode *inp) {

}
