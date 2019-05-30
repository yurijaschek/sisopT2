/*****************************************************************************
 *  Instituto de Informatica - Universidade Federal do Rio Grande do Sul     *
 *  INF01142 - Sistemas Operacionais I N                                     *
 *  Task 2 File System (T2FS) 2019/1                                         *
 *                                                                           *
 *  Authors: Yuri Jaschek                                                    *
 *           Giovane Fonseca                                                 *
 *           Humberto Lentz                                                  *
 *           Matheus F. Kovaleski                                            *
 *                                                                           *
 *****************************************************************************/

/*
 *   T2FS API functions
 */

#include "libt2fs.h"
#include "t2fs.h"

#include <stdio.h>

static const int partition = 0;

/************************************
 *  External variables definitions  *
 ************************************/

struct t2fs_superblock superblock;


/************************
 *  API open functions  *
 ************************/

int identify2 (char *name, int size)
{
    char comp[][2][32] =
    {
        {"00231592", "Yuri Jaschek"},
        {"00243451", "Giovane Fonseca"},
        {"00242308", "Humberto Lentz"},
        {"00274744", "Matheus F. Kovaleski"},
    };

    for(int i=0; i<ARRAY_SIZE(comp); i++)
    {
        int n = snprintf(name, size, "{\"%s\", \"%s\"}%s", comp[i][1],
                         comp[i][0], i == ARRAY_SIZE(comp)-1 ? "" : ", ");
        name += n;
        size -= n;
        if(size <= 0)
            return -1;
    }

	return 0;
}


int format2 (int sectors_per_block)
{
    // We shouldn't call t2fs_init() in this function, since
    //   this function doesn't expect the partition to already be formatted

    if(sectors_per_block < 1 || sectors_per_block > 128) // Max allowed is 128
        return -1;

    return init_format(sectors_per_block, partition); // Format partition
}


FILE2 create2 (char *path)
{
    (void)path;
    return -1;
}


int delete2 (char *path)
{
    (void)path;
    return -1;
}


FILE2 open2 (char *path)
{
    (void)path;
    return -1;
}


int close2 (FILE2 handle)
{
    (void)handle;
    return -1;
}


int read2 (FILE2 handle, char *buffer, int size)
{
    (void)handle;
    (void)buffer;
    (void)size;
    return -1;
}


int write2 (FILE2 handle, char *buffer, int size)
{
    (void)handle;
    (void)buffer;
    (void)size;
    return -1;
}


int truncate2 (FILE2 handle)
{
    (void)handle;
    return -1;
}


int seek2 (FILE2 handle, uint32_t offset)
{
    (void)handle;
    (void)offset;
    return -1;
}


int mkdir2 (char *path)
{
    (void)path;
    return -1;
}


int rmdir2 (char *path)
{
    (void)path;
    return -1;
}


int chdir2 (char *path)
{
    (void)path;
    return -1;
}


int getcwd2 (char *path, int size)
{
    (void)path;
    (void)size;
    return -1;
}


DIR2 opendir2 (char *path)
{
    (void)path;
    return -1;
}


int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    (void)handle;
    (void)dentry;
    return -1;
}


int closedir2 (DIR2 handle)
{
    (void)handle;
    return -1;
}


int ln2 (char *linkpath, char *pointpath)
{
    (void)linkpath;
    (void)pointpath;
    return -1;
}

