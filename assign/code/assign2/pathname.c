
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PATH_SEP "/"

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
 
}

int pathname_lookup_helper(struct unixfilesystem *fs, const int dirinumber,
                           char *tok, struct direntv6 *dirEnt) {
 
}
