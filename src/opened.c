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
 *   Opened files and descriptor functions
 */

#include "libt2fs.h"
#include <stdlib.h>
#include <string.h>


/************************
 *  Internal variables  *
 ************************/

// Position 0 is reserved to directory descriptor
// Positions 1 to T2FS_MAX_FILES_OPENED is for regular files
static struct t2fs_descriptor table[1+T2FS_MAX_FILES_OPENED];
static int fd_counter;


/************************
 *  Internal functions  *
 ************************/

static u32 get_nth_block_indirect(u32 *block, int level, int count)
{
    if(level > 0) // block is index block pointer
    {
        byte_t *buffer = malloc(superblock.block_size);
        if(!buffer)
            return 0;

        if(*block == 0) // Index block unallocated
            return 0;
        if(t2fs_read_block(buffer, *block) != 0)
            return 0;

        u32 level_blocks = 1;
        for(int i=0; i<level-1; i++)
            level_blocks *= superblock.block_size / sizeof(u32);

        return get_nth_block_indirect(&((u32*)buffer)[count/level_blocks],
                                      level-1, count % level_blocks);
    }
    else // block is data block pointer
    {
        return *block;
    }
}


/*-----------------------------------------------------------------------------
Funct:  Given an opened inode, return its nth allocated block.
Input:  inode -> Pointer to the opened inode
        n     -> The data block index N, starting from 0
Return: On success, the block number is returned.
        Otherwise, if the Nth block is not allocated, return 0.
-----------------------------------------------------------------------------*/
static u32 get_nth_block(struct t2fs_inode *inode, int n)
{
    if(n < NUM_DIRECT_PTR)
        return inode->pointers[n];

    if(NUM_INDIRECT_LVL == 0) // No indirection
        return 0;

    u32 rem_blocks = n - NUM_DIRECT_PTR;
    u32 level_blocks = 1;
    for(int i=0; i<NUM_INDIRECT_LVL; i++)
    {
        level_blocks *= superblock.block_size / sizeof(u32);
        if(rem_blocks < level_blocks)
        {
            return get_nth_block_indirect(&inode->pointers[NUM_DIRECT_PTR+i],
                                    i+1, rem_blocks);
        }
        rem_blocks -= level_blocks;
    }

    return 0;
}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Find a free descriptor to use with the given inode of the given type.
Input:  inode -> Inode of the file to have a descriptor
        type  -> Type of the given inode
Return: On success, the address of the descriptor used (in table) is returned.
        Otherwise, if the table is full, NULL is returned.
-----------------------------------------------------------------------------*/
struct t2fs_descriptor *get_new_desc(u32 inode, u8 type)
{
    int pos = -1;
    if(type == FILETYPE_DIRECTORY)
        pos = table[0].id == 0 ? 0 : -1; // Only one spot for directories
    else
    for(int i=1; i<=T2FS_MAX_FILES_OPENED; i++)
    {
        if(table[i].id == 0) // Found a free spot in the table
        {
            pos = i;
            break;
        }
    }
    if(pos == -1)
        return 0; // NULL
    table[pos].id = ++fd_counter;
    table[pos].type = type;
    table[pos].curr_pos = 0;
    table[pos].inode = inode;
    return &table[pos];
}


/*-----------------------------------------------------------------------------
Funct:  Find the descriptor being used in table, having the given id.
Input:  fd -> The identification of the descriptor to be found
Return: On success, the address of the descriptor (in table) is returned.
        Otherwise, if the given id was not on the table, NULL is returned.
-----------------------------------------------------------------------------*/
struct t2fs_descriptor *find_desc(int id)
{
    for(int i=0; i<=T2FS_MAX_FILES_OPENED; i++)
    {
        if(table[i].id == id)
            return &table[i];
    }
    return 0; // NULL
}


/*-----------------------------------------------------------------------------
Funct:  Release the descriptor, making it usable for other files again.
Input:  fd -> Pointer to the descriptor, which must have been got by a call to
              find_desc, or other way, as long as it's in table.
-----------------------------------------------------------------------------*/
void release_desc(struct t2fs_descriptor *fd)
{
    memset(fd, 0, sizeof(*fd));
}


/*-----------------------------------------------------------------------------
Funct:  Close all files that are using the given inode.
        This function is necessary when all entry references to the given inode
            are deleted, prompting the deallocation of the inode and the
            logic blocks being used by the inode.
Input:  inode -> Inode to be searched for
-----------------------------------------------------------------------------*/
void close_all_inode(u32 inode)
{
    for(int i=0; i<=T2FS_MAX_FILES_OPENED; i++)
    {
        if(table[i].inode == inode)
            release_desc(&table[i]);
    }
}


/*-----------------------------------------------------------------------------
Funct:  Adjust the current position of all file descriptors with the given
            inode for it to not be greater than the given limit.
        This function must be called when truncating a file.
Input:  inode -> Inode to be searched for
        limit -> The current position limit (new size of the file)
-----------------------------------------------------------------------------*/
void adjust_pointer_all(u32 inode, u32 limit)
{
    for(int i=0; i<=T2FS_MAX_FILES_OPENED; i++)
    {
        if(table[i].inode == inode)
            table[i].curr_pos = MIN(table[i].curr_pos, limit);
    }
}


/*-----------------------------------------------------------------------------
Funct:  Read from or write to a file.
Input:  buffer   -> Where to put data read or to get data from if writing
        inode    -> Inode of the file to be read/written
        curr_pos -> Current position on the file to operate
        size     -> Number of bytes to read/write
        wr       -> If the operation is read (false) or write (true)
Return: On success, the number of bytes read/written is returned.
        On error, a negative value is returned.
-----------------------------------------------------------------------------*/
int t2fs_rw_data(byte_t *buffer, u32 inode, u32 curr_pos, u32 size, bool wr)
{
    struct t2fs_inode inode_s;
    if(read_inode(inode, &inode_s) != 0)
        return -1;

    u32 rem = size;
    while(rem > 0)
    {
        u32 offset = curr_pos % superblock.block_size; // Offset in the block
        // Operations are done one block each time. Number of bytes to operate
        u32 bytes = MIN(rem, superblock.block_size - offset);
        if(!wr) // Only if reading, respect the size of the file
            bytes = MIN(bytes, inode_s.bytes_size - curr_pos);
        if(bytes == 0) // Nothing to be done
            break;

        u32 block = get_nth_block(&inode_s, curr_pos / superblock.block_size);
        if(block == 0 && !wr)
            break;
        if(block == 0) // Writing: Need to allocate a new block
        {
            block = allocate_new_block(inode);
            if(block == 0)
                break;
            // Because the inode was written in allocate_new_block
            if(read_inode(inode, &inode_s) != 0)
                break;
        }

        if(t2fs_read_block(block_buffer, block) != 0)
            break;
        if(wr) // Write operation
        {
            memcpy(block_buffer+offset, buffer, bytes);
            if(t2fs_write_block(block_buffer, block) != 0)
                break;
        }
        else // Read operation
            memcpy(buffer, block_buffer+offset, bytes);

        rem -= bytes;
        buffer += bytes;
        curr_pos += bytes;
        if(wr) // To handle the file getting larger
            inode_s.bytes_size = MAX(inode_s.bytes_size, curr_pos);
    }

    if(wr) // To handle file getting larger
        write_inode(inode, &inode_s);

    return size - rem;
}
