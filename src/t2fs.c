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


/********************************
 *  Structures print functions  *
 ********************************/

void print_superblock(struct t2fs_superblock *sblock)
{
    printf("t2fs_superblock:\n");
    printf("    sectors_per_block : %u\n", sblock->sectors_per_block);
    printf("    signature         : %s\n", sblock->signature);
    printf("    first_sector      : %u\n", sblock->first_sector);
    printf("    num_sectors       : %u\n", sblock->num_sectors);
    printf("    num_blocks        : %u\n", sblock->num_blocks);
    printf("    num_inodes        : %u\n", sblock->num_inodes);
    printf("    it_offset         : %u\n", sblock->it_offset);
    printf("    ib_offset         : %u\n", sblock->ib_offset);
    printf("    bb_offset         : %u\n", sblock->bb_offset);
    printf("    blocks_offset     : %u\n", sblock->blocks_offset);
}

void print_inode(u32 number, struct t2fs_inode *inode)
{
    printf("t2fs_inode <%u>:\n", number);
    printf("    type       : %u\n", inode->type);
    printf("    hl_count   : %u\n", inode->hl_count);
    printf("    num_blocks : %u\n", inode->num_blocks);
    printf("    bytes_size : %u\n", inode->bytes_size);
    printf("    pointers   :\n");
    for(int i=0; i<NUM_INODE_PTR; i++)
        printf("        [%02d]   : %u\n", i, inode->pointers[i]);
}

void print_path(struct t2fs_path *path_info)
{
    printf("t2fs_path:\n");
    printf("    valid     : %u\n", path_info->valid);
    printf("    exists    : %u\n", path_info->exists);
    printf("    type      : %u\n", path_info->type);
    printf("    name      : %s\n", path_info->name);
    printf("    par_inode : %u\n", path_info->par_inode);
    printf("    inode     : %u\n", path_info->inode);
}


/************************************
 *  External variables definitions  *
 ************************************/

// All are initialized in t2fs_init
struct t2fs_superblock superblock;
u32 cwd_inode;
char cwd_path[T2FS_PATH_MAX];
byte_t *block_buffer;


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
    // We shouldn't call t2fs_init() in the beginning of this function, since
    //   this function doesn't expect the partition to already be formatted

    if(sectors_per_block < 1 || sectors_per_block > 128) // Max allowed is 128
        return -1;

    int res = init_format(sectors_per_block, partition); // Format partition
    if(res != 0)
        return res;

    res = init_t2fs(partition); // Initialize before calling functions
    if(res != 0)
        return res;

    if(find_new_inode(FILETYPE_DIRECTORY) != ROOT_INODE) // Should be 1
        return -1;

    res = insert_entry(ROOT_INODE, ".", ROOT_INODE);
    if(res != 0)
        return res;
    res = insert_entry(ROOT_INODE, "..", ROOT_INODE);
    if(res != 0)
        return res;

    return 0;
}


FILE2 create2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int delete2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


FILE2 open2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int close2 (FILE2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    return -1;
}


int read2 (FILE2 handle, char *buffer, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    (void)buffer;
    (void)size;
    return -1;
}


int write2 (FILE2 handle, char *buffer, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    (void)buffer;
    (void)size;
    return -1;
}


int truncate2 (FILE2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    return -1;
}


int seek2 (FILE2 handle, uint32_t offset)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    (void)offset;
    return -1;
}


int mkdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int rmdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int chdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int getcwd2 (char *path, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    (void)size;
    return -1;
}


DIR2 opendir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)path;
    return -1;
}


int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    (void)dentry;
    return -1;
}


int closedir2 (DIR2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)handle;
    return -1;
}


int ln2 (char *linkpath, char *pointpath)
{
    if(init_t2fs(partition) != 0) return -1;
    (void)linkpath;
    (void)pointpath;
    return -1;
}
