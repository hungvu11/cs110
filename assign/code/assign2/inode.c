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
    int offset = (inumber - 1) / INODE_PER_BLOCK;
    struct inode buffer[INODE_PER_BLOCK];
    int byteRead = diskimg_readsector(fs->dfd, INODE_START_SECTOR + offset, buffer);
    if (byteRead != DISKIMG_SECTOR_SIZE) {
        fprintf(stderr, "Failed to get inode number %d.\n", inumber);
        return -1;
    }
    *inp = buffer[(inumber - 1) % INODE_PER_BLOCK];
    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if ((inp->i_mode & ILARG) == 0) {
        // inode using algorithm for small files
        return inp->i_addr[blockNum];
    }
    uint16_t buffer[BLOCKS_PER_INDIR];
    // inode using algorithm for large files
    if (blockNum < TOTAL_BLOCKS_FROM_INDIR) {
        // using indirect blocks
        uint16_t block_addr = inp->i_addr[blockNum / BLOCKS_PER_INDIR];
        int byteRead = diskimg_readsector(fs->dfd, block_addr, buffer);
        if (byteRead != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, "Fail to lookup for a file block.\n");
            return -1;
        }
        return buffer[blockNum % BLOCKS_PER_INDIR];
    } else {
        // doubly indirect block
        blockNum -= TOTAL_BLOCKS_FROM_INDIR;
        int byteRead = diskimg_readsector(fs->dfd, inp->i_addr[7], buffer);
        uint16_t block_add = buffer[blockNum / BLOCKS_PER_INDIR];
        byteRead = diskimg_readsector(fs->dfd, block_add, buffer);
        return buffer[blockNum % BLOCKS_PER_INDIR];
    }
    return -1;
}

int inode_getsize(struct inode *inp) {
    return (inp->i_size1 | (inp->i_size0 << 16));
}
