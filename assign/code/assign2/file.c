#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode inp;
    if (inode_iget(fs, inumber, &inp) < 0) return -1;
    int block_add;
    if ((block_add = inode_indexlookup(fs, &inp, blockNum)) < 0) return -1;
    if (diskimg_readsector(fs->dfd, block_add, buf) < 0) return -1;

    int size = inode_getsize(&inp);
    int total_block = size / DISKIMG_SECTOR_SIZE;
    if (blockNum == total_block) {
        return size % DISKIMG_SECTOR_SIZE;
    } else {
        return DISKIMG_SECTOR_SIZE;
    }
}
