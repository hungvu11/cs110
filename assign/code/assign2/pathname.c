
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PATH_SEP "/"

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (strcmp(pathname, "/") == 0) return ROOT_INUMBER; // root directory

    char *filepath = strdup(pathname);
    if (filepath == NULL) return -1;
    char *token = strsep(&filepath, "/");
    if (strlen(token) > 0) return -1;
    
    int dirinumber = ROOT_INUMBER;
    struct direntv6 dirEnt;
    while ((token = strsep(&filepath, "/")) != NULL) {
        if (directory_findname(fs, token, dirinumber, &dirEnt) == -1) return -1;
        dirinumber = dirEnt.d_inumber;
    }
    return dirinumber;
}

