#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
	struct inode in;
	if (inode_iget(fs, dirinumber, &in) < 0) return -1;
	if ((in.i_mode & IFMT) != IFDIR || (in.i_mode & IALLOC) == 0) {
		fprintf(stderr, "File with inumber %d is not a directory or not allocated.\n", dirinumber);
		return -1;
	}
	int size = inode_getsize(&in);
	int num_block = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
	struct direntv6 buf[DISKIMG_SECTOR_SIZE / sizeof(struct direntv6)];
	for (int block_num=0; block_num < num_block; block_num++) {
		// get content from each block in directory
		int read_bytes = file_getblock(fs, dirinumber, block_num, buf);
		if (read_bytes < 0) return -1;
		int num_file = read_bytes / sizeof(struct direntv6);
		for (int i=0; i<num_file; i++) {
			if (strncmp(buf[i].d_name, name, 14) == 0) {
				*dirEnt = buf[i];
				return 0;
			}
		}
	}
	fprintf(stderr, "No file with matching name found in the directory.\n");
	return -1;
}
