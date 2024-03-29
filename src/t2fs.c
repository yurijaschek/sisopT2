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
#include <string.h>

static const int partition = 0;


/********************************
 *  Structures print functions  *
 ********************************/

void print_mbr(struct t2fs_mbr *mbr)
{
    printf("t2fs_mbr:\n");
    printf("    version     : %u\n", mbr->version);
    printf("    sector_size : %u\n", mbr->sector_size);
    printf("    pt_offset   : %u\n", mbr->pt_offset);
    printf("    pt_entries  : %u\n", mbr->pt_entries);
    for(u16 i=0; i<mbr->pt_entries; i++)
    {
        printf("    ptable[%u]:\n", i);
        printf("        first_sector : %u\n", mbr->ptable[i].first_sector);
        printf("        last_sector  : %u\n", mbr->ptable[i].last_sector);
        printf("        name         : %s\n", mbr->ptable[i].name);
    }
}

void print_superblock(struct t2fs_superblock *sblock)
{
    printf("t2fs_superblock:\n");
    printf("    sectors_per_block : %u\n", sblock->sectors_per_block);
    printf("    signature         : %s\n", sblock->signature);
    printf("    sector_size       : %u\n", sblock->sector_size);
    printf("    block_size        : %u\n", sblock->block_size);
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
    printf("    bytes_size : %u\n", inode->bytes_size);
    printf("    num_blocks : %u\n", inode->num_blocks);
    printf("    pointers   :\n");
    for(int i=0; i<NUM_INODE_PTR; i++)
        printf("        [%02d]   : %u\n", i, inode->pointers[i]);
}

void print_descriptor(struct t2fs_descriptor *desc)
{
    printf("t2fs_descriptor:\n");
    printf("    id       : %d\n", desc->id);
    printf("    type     : %u\n", desc->type);
    printf("    curr_pos : %u\n", desc->curr_pos);
    printf("    inode    : %u\n", desc->inode);
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

// All are initialized in init_t2fs
struct t2fs_superblock superblock;
byte_t *block_buffer;
u32 *idx_block_buffer[NUM_INDIRECT_LVL];
u32 cwd_inode;


/************************************
 *  Internal variables definitions  *
 ************************************/

static struct t2fs_path info; // To get path information
static struct t2fs_descriptor *fd; // To get descriptors for files


/************************
 *  API open functions  *
 ************************/

int identify2 (char *name, int size)
{
    char comp[][32] =
    {
        "Yuri Jaschek",
        "Giovane Fonseca",
        "Humberto Lentz",
        "Matheus F. Kovaleski",
    };

    for(int i=0; i<ARRAY_SIZE(comp); i++)
    {
        int n = snprintf(name, size, "{\"%s\"}%s", comp[i],
                         i == ARRAY_SIZE(comp)-1 ? "" : ", ");
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

    if(use_new_inode(FILETYPE_DIRECTORY) != ROOT_INODE) // Should be 1
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
    info = get_path_info(path, true);
    if(!info.valid || (info.exists && info.type != FILETYPE_REGULAR))
        return -1;

    u32 inode = info.inode;
    if(!info.exists) // Need to be created
    {
        inode = use_new_inode(FILETYPE_REGULAR);
        if(inode == 0)
            return -1;

        int res = insert_entry(info.par_inode, info.name, inode);
        if(res != 0)
            return -1;
    }

    fd = get_new_desc(inode, FILETYPE_REGULAR);
    if(!fd)
        return -1;

    if(info.exists)
        truncate2(fd->id);

    return fd->id;
}


int delete2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, false);
    if(!info.exists || info.type == FILETYPE_DIRECTORY)
        return -1;

    return delete_entry(info.par_inode, info.name);
}


FILE2 open2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, true);
    if(!info.exists || info.type != FILETYPE_REGULAR)
        return -1;

    fd = get_new_desc(info.inode, FILETYPE_REGULAR);
    if(!fd)
        return -1;

    return fd->id;
}


int close2 (FILE2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_REGULAR)
        return -1;

    release_desc(fd);
    return 0;
}


int read2 (FILE2 handle, char *buffer, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_REGULAR)
        return -1;

    if(size < 0)
        return -1;
    if(size == 0)
        return 0; // 0 bytes read

    int ans = t2fs_rw_data((byte_t*)buffer, fd->inode,
                           fd->curr_pos, size, false);
    if(ans >= 0)
        fd->curr_pos += ans; // Advance current position in file

    return ans;
}


int write2 (FILE2 handle, char *buffer, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_REGULAR)
        return -1;

    if(size < 0)
        return -1;
    if(size == 0)
        return 0; // 0 bytes written

    int ans = t2fs_rw_data((byte_t*)buffer, fd->inode,
                           fd->curr_pos, size, true);
    if(ans >= 0)
        fd->curr_pos += ans; // Advance current position in file

    return ans;
}


int truncate2 (FILE2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_REGULAR)
        return -1;

    struct t2fs_inode inode;
    if(read_inode(fd->inode, &inode) != 0)
        return -1;

    inode.bytes_size = fd->curr_pos;
    if(write_inode(fd->inode, &inode) != 0)
        return -1;

    int num = -1; // Number of blocks for deallocation (-1 = all blocks)
    if(fd->curr_pos != 0) // The formula below doesn't work for curr_pos = 0
        num = inode.num_blocks - (1 + (fd->curr_pos-1)/superblock.block_size);

    adjust_pointer_all(fd->inode, fd->curr_pos); // To prevent hazards
    return deallocate_blocks(fd->inode, num);
}


int seek2 (FILE2 handle, uint32_t offset)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_REGULAR)
        return -1;

    struct t2fs_inode inode;
    if(read_inode(fd->inode, &inode) != 0)
        return -1;

    if((s32)offset == -1)
        fd->curr_pos = inode.bytes_size;
    else if(offset <= inode.bytes_size)
        fd->curr_pos = offset;
    else
        return -1;
    return 0;
}


int mkdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, true);
    if(!info.valid || info.exists)
        return -1;

    u32 inode = use_new_inode(FILETYPE_DIRECTORY);
    if(inode == 0)
        return -1;

    int res;
    res = insert_entry(inode, ".", inode);
    if(res != 0)
        return -1;
    res = insert_entry(inode, "..", info.par_inode);
    if(res != 0)
        return -1;
    res = insert_entry(info.par_inode, info.name, inode);

    return 0;
}


int rmdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, false);
    if(!info.exists || info.type != FILETYPE_DIRECTORY)
        return -1;

    if(info.inode == ROOT_INODE || info.inode == cwd_inode)
        return -1;
    if(!dir_deletable(info.inode))
        return -1;

    int res;
    res = delete_entry(info.inode, "..");
    if(res != 0)
        return res;
    res = delete_entry(info.inode, ".");
    if(res != 0)
        return res;

    return delete_entry(info.par_inode, info.name);
}


int chdir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, true);
    if(!info.exists || info.type != FILETYPE_DIRECTORY)
        return -1;

    cwd_inode = info.inode;
    return 0;
}


int getcwd2 (char *path, int size)
{
    if(init_t2fs(partition) != 0) return -1;
    char cwd_path[T2FS_PATH_MAX] = "";
    u32 temp = cwd_inode;

    while(cwd_inode != ROOT_INODE)
    {
        info = get_path_info("..", false);
        char name[T2FS_FILENAME_MAX+1] = "/";
        if(get_name_by_inode(info.inode, name+1, cwd_inode) != 0)
        {
            printf("IXI!\n");
            return -1;
        }
        reverse_string(name);
        strncat(cwd_path, name, T2FS_PATH_MAX);
        cwd_inode = info.inode;
    }

    cwd_inode = temp;

    if(strlen(cwd_path) == 0) // Root directory
        strcpy(cwd_path, "/");
    reverse_string(cwd_path);
    strncpy(path, cwd_path, size);
    return 0;
}


DIR2 opendir2 (char *path)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(path, true);
    if(!info.exists || info.type != FILETYPE_DIRECTORY)
        return -1;

    fd = get_new_desc(info.inode, FILETYPE_DIRECTORY);
    if(!fd)
        return -1;

    return fd->id;
}


int readdir2 (DIR2 handle, DIRENT2 *dentry)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_DIRECTORY)
        return -1;

    struct t2fs_record record = {};
    int res;
    for(;;)
    {
        // Find next entry
        res = t2fs_rw_data((byte_t*)&record, fd->inode, fd->curr_pos,
                           sizeof(struct t2fs_record), false);
        if(res <= 0) // Reached end of directory
            return -1;
        fd->curr_pos += res;
        // Test if a record can fit at the end of the block
        u32 next_block_pos = superblock.block_size
                             * (1 + fd->curr_pos / superblock.block_size);
        if((next_block_pos - fd->curr_pos) / sizeof(struct t2fs_record) < 1)
            fd->curr_pos = next_block_pos; // Couldn't fit

        if(record.inode != 0) // Found valid entry
            break;
    }

    struct t2fs_inode inode;
    if(read_inode(record.inode, &inode) != 0)
        return -1;

    strcpy(dentry->name, record.name);
    dentry->fileType = inode.type;
    dentry->fileSize = inode.bytes_size;

    return 0;
}


int closedir2 (DIR2 handle)
{
    if(init_t2fs(partition) != 0) return -1;
    fd = find_desc(handle);
    if(!fd || fd->type != FILETYPE_DIRECTORY)
        return -1;

    release_desc(fd);
    return 0;
}


int ln2 (char *linkpath, char *pointpath)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(linkpath, false);
    if(!info.valid || (info.exists && info.type != FILETYPE_SYMLINK))
        return -1;

    u32 inode = info.inode;
    if(!info.exists) // Need to be created
    {
        inode = use_new_inode(FILETYPE_SYMLINK);
        if(inode == 0)
            return -1;

        int res = insert_entry(info.par_inode, info.name, inode);
        if(res != 0)
            return -1;

        if(allocate_new_block(inode) == 0)
            return -1;
    }

    struct t2fs_inode inode_s;
    if(read_inode(inode, &inode_s) != 0)
        return -1;

    memset(block_buffer, 0, superblock.block_size);
    strncpy((char*)block_buffer, pointpath, superblock.block_size);
    return t2fs_write_block(block_buffer, inode_s.pointers[0]);
}


int hardln2(char *linkpath, char *pointpath)
{
    if(init_t2fs(partition) != 0) return -1;
    info = get_path_info(pointpath, false);
    if(!info.exists || info.type == FILETYPE_DIRECTORY)
        return -1;

    u32 inode = info.inode;
    info = get_path_info(linkpath, false);
    if(!info.valid || info.exists)
        return -1;

    return insert_entry(info.par_inode, info.name, inode);
}
