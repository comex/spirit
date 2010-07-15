#include "hfslib.h"
#include <stdio.h>

Volume *volume;

static char exists(char *path) {
    printf("exists? %s\n", path);
    char ret = NULL != getRecordFromPath(path, volume, NULL, NULL);
    if(!ret) {
        printf("%s does not exist; creating\n", path);
    }
    return ret;
}

int main() {
    AbstractFile *empty = createAbstractFileFromMemory(NULL, 0);
    io_func *io = openFlatFile("/dev/rdisk0s1");
    volume = openVolume(io);
    printf("volume = %x\n", volume);
    hfs_ls(volume, "/");
    hfs_ls(volume, "/private");
    hfs_ls(volume, "/private/var");
    hfs_ls(volume, "/private/var/db");
    hfs_ls(volume, "/private/etc");
    /*io->close(io);
    return 0;*/
    if(!exists("/private/var"))
        newFolder("/private/var", volume);
    if(!exists("/private/var/db"))
        newFolder("/private/var/db", volume);
    if(!exists("/private/var/db/.launchd_use_gmalloc"))
        add_hfs(volume, empty, "/private/var/db/.launchd_use_gmalloc");

    closeVolume(volume);
    io->close(io);
    return 0;
}
