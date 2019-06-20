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
 *   File allocation and management functions
 */

#include "libt2fs.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


/************************
 *  Internal functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Given an inode number, calculate the sector in the inodes table it's
            in and the offset (in bytes) where the structure starts.
Input:  inode  -> The inode number given
        sector -> The sector the inode is in
        byte   -> The offset in the sector where the inode is
-----------------------------------------------------------------------------*/
static void calculate_inode_table(u32 inode, u32 *sector, int *byte)
{
    int inodes_per_sector = superblock.sector_size / sizeof(struct t2fs_inode);
    *sector = superblock.it_offset + inode / inodes_per_sector;
    *byte = (inode % inodes_per_sector) * sizeof(struct t2fs_inode);
}


/*-----------------------------------------------------------------------------
Funct:  Operate either the inodes bitmap or the blocks bitmap.
        The following operations are permitted:
            -1 : check if the given inode/block is being used
             0 : mark the given inode/block as being free (clear)
             1 : mark the given inode/block as being used (set)
Input:  number    -> The given inode/block
        inode     -> If the number is an inode (true) or a block (false)
        operation -> The operation to be performed
Return: On success with operation -1, the return is 0 if the inode/block is not
            being used and positive if it's being used.
        On success with operations 0 and 1, the return is 0.
        On error with any operation, the return is negative.
-----------------------------------------------------------------------------*/
static int operate_bitmap(u32 number, bool inode, int operation)
{
    u32 sector = number / (8 * superblock.sector_size);
    sector += inode ? superblock.ib_offset : superblock.bb_offset;
    int byte = number % (8 * superblock.sector_size);
    int bit = number % 8;

    byte_t data;
    if(t2fs_read_sector(&data, sector, byte, 1) != 0)
        return -1;

    if(operation == -1) // Check
        return CHK_BIT(data, bit);

    if(operation == 0) // Clear
        CLR_BIT(data, bit);
    else // Set (operation == 1)
        SET_BIT(data, bit);

    if(t2fs_write_sector(&data, sector, byte, 1) != 0)
        return -1;

    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Search for the first inode or block that is free.
Input:  inode -> What is to be searched: inodes (true) or blocks (false)
Return: On success, returns the first inode/block found (positive integer).
        If there are no free inodes/blocks, 0 is returned.
-----------------------------------------------------------------------------*/
static u32 first_free(bool inode)
{
    u32 num = inode ? superblock.num_inodes : superblock.num_blocks;
    for(u32 i=1U; i<num; i++)
    {
        // If operate_bitmap returns an error (-1), the if will be false
        // It is safer to not use it in case of error
        if(!operate_bitmap(i, inode, -1))
            return i;
    }

    return 0; // No free inodes/blocks available
}


/*-----------------------------------------------------------------------------
Funct:  Find a new free block to use, marking it as used, if found.
Return: On success, returns the block number (positive integer).
        If there are no blocks available, 0 is returned.
-----------------------------------------------------------------------------*/
static u32 find_new_block()
{
    u32 ans = first_free(false);
    if(ans != 0)
    {
        if(operate_bitmap(ans, false, 1) != 0)
            return 0;
    }
    return ans;
}


/*-----------------------------------------------------------------------------
Funct:  Given a data block and its indirection level, apply a given function to
            each of its composing data blocks, in ascending order.
        Please, refer to iterate_inode_blocks function for details about the
            application of the given function.
        This function helps to implement the different indirection levels of
            how blocks are allocated with inodes, part of iterate_inode_blocks.
Input:  block -> Block to be iterated
        level -> Level of indirection (0 = direct; 1 = singly; 2 = doubly; etc)
        fn    -> The function that should be applied to each block
        args  -> The additional arguments
Return: Same return as specified in the iterate_inode_blocks function.
-----------------------------------------------------------------------------*/
static int iterate_indirection(u32 block, int level,
                               int (*fn)(u32, va_list), va_list args)
{
    if(level == 0)
        return fn(block, args);

    byte_t *data = malloc(superblock.block_size);
    if(!data)
        return -1;
    int res = t2fs_read_block(data, block);
    if(res != 0)
        return res;

    int num_iter = superblock.block_size / sizeof(u32);
    u32 *it = (u32*)data;
    for(int i=0; i<num_iter; i++)
    {
        res = 1;
        if(it[i] == 0)
            break;
        va_list a;
        va_copy(a, args);
        res = iterate_indirection(it[i], level-1, fn, a);
        if(res <= 0)
            break;
    }
    free(data);
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Allocate a new indirect block.
        The parameter level controls whether the block variable is a block
            index (> 0) or an address where to put the new block (= 0).
        If block is 0 and there are still indirection, the function allocates
            a new index block additionally.
Input:  block -> Pointer to index block or where to store the newly allocated
        level -> Level of indirection (0 = direct; 1 = singly; 2 = doubly; etc)
        count -> Number of blocks before the one to be allocated (level-wise)
Return: On success, the data block allocated is returned.
        Otherwise, 0 (invalid block) is returned.
-----------------------------------------------------------------------------*/
static u32 allocate_indirect(u32 *block, int level, u32 count)
{
    if(level > 0) // block is index block pointer
    {
        byte_t *buffer = malloc(superblock.block_size);
        if(!buffer)
            return 0;

        if(*block == 0) // Index block unallocated
        {
            *block = find_new_block();
            if(*block == 0)
                return 0;
            memset(buffer, 0, superblock.block_size); // All invalid pointers
        }
        else
        {
            if(t2fs_read_block(buffer, *block) != 0)
                return 0;
        }

        u32 level_blocks = 1;
        for(int i=0; i<level-1; i++)
            level_blocks *= superblock.block_size / sizeof(u32);
        u32 ans = allocate_indirect(&((u32*)buffer)[count/level_blocks],
                                    level-1, count % level_blocks);

        if(t2fs_write_block(buffer, *block) != 0)
            return 0;

        return ans;
    }
    else // block is data block pointer
    {
        return *block = find_new_block();
    }
}


/*-----------------------------------------------------------------------------
Funct:  Deallocate an indirect block being used.
        The parameter level controls whether the block variable is a block
            index (> 0) or an address of the block to be deallocated (= 0).
        If there are no more blocks in the index block, the function
            deallocates the index block additionally.
Input:  block -> Pointer to index block or where is the block to be deallocated
        level -> Level of indirection (0 = direct; 1 = singly; 2 = doubly; etc)
        count -> Number of blocks before the one to be deallocated (level-wise)
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
static int deallocate_indirect(u32 *block, int level, u32 *count)
{
    if(*block == 0 || *count == 0) // Nothing to be done
        return 0;

    int res;
    if(level == 0) // block is data block pointer
    {
        res = operate_bitmap(*block, false, 0);
        if(res != 0)
            return res;
        *block = 0;
        (*count)--;
        return 0;
    }
    else // block is index block pointer
    {
        byte_t *buffer = malloc(superblock.block_size);
        if(!buffer)
            return -1;

        res = t2fs_read_block(buffer, *block);
        if(res != 0)
            return res;

        for(int i=superblock.block_size/sizeof(u32)-1; i>=0 && *count>0; i--)
        {
            res = deallocate_indirect(&((u32*)buffer)[i], level-1, count);
            if(res != 0)
                return res;
        }

        if(((u32*)buffer)[0] == 0) // Empty index block
        {
            operate_bitmap(*block, false, 0);
            if(res != 0)
                return res;
            *block = 0;
        }
        else
        {
            res = t2fs_write_block(buffer, *block);
            if(res != 0)
                return res;
        }

        return 0;
    }
}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Read the given inode from disk (inodes table) to memory.
Input:  inode -> The given inode to be read
        data  -> Where to store the inode information read
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int read_inode(u32 inode, struct t2fs_inode *data)
{
    if(inode == 0 || inode >= superblock.num_inodes)
        return -1;

    u32 sector;
    int byte;
    calculate_inode_table(inode, &sector, &byte);

    int res = t2fs_read_sector((byte_t*)data, sector, byte,
                               sizeof(struct t2fs_inode));
    if(res != 0)
        return res;

    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Write the given inode from memory to disk (inodes table).
Input:  inode -> The given inode to be written
        data  -> Where the inode information to be written is
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int write_inode(u32 inode, struct t2fs_inode *data)
{
    if(inode == 0 || inode >= superblock.num_inodes)
        return -1;

    u32 sector;
    int byte;
    calculate_inode_table(inode, &sector, &byte);

    int res = t2fs_write_sector((byte_t*)data, sector, byte,
                                sizeof(struct t2fs_inode));
    if(res != 0)
        return res;

    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Find a new free inode to use.
Input:  type -> Type of the file the inode corresponds to
Return: On success, returns the inode number (positive integer).
        If there are no inodes available, 0 is returned.
-----------------------------------------------------------------------------*/
u32 use_new_inode(u8 type)
{
    u32 inode = first_free(true);
    if(inode != 0)
    {
        struct t2fs_inode data = {}; // The new inode structure
        data.type = type;
        // Other inode data will be updated as the inode is modified

        int res = write_inode(inode, &data); // Stores the inode on disk
        if(res != 0)
            return res;

        if(operate_bitmap(inode, true, 1) != 0) // Updates the bitmap
            return 0;
    }

    return inode;
}


/*-----------------------------------------------------------------------------
Funct:  Allocate a new block for an inode to use.
Input:  inode -> The inode that needs another block
Return: On success, returns the block number (positive integer).
        If there are no blocks available, 0 is returned.
-----------------------------------------------------------------------------*/
u32 allocate_new_block(u32 inode)
{
    struct t2fs_inode inode_s;
    int res = read_inode(inode, &inode_s);
    if(res != 0)
        return 0;
    u32 ans = 0;
    if(inode_s.num_blocks < NUM_DIRECT_PTR) // Can allocate direct pointer
        ans = allocate_indirect(&inode_s.pointers[inode_s.num_blocks], 0, 0);
    else // Need to check indirect pointers
    {
        if(NUM_INDIRECT_LVL == 0) // No indirection
            return 0;
        u32 rem_blocks = inode_s.num_blocks - NUM_DIRECT_PTR;
        u32 level_blocks = 1;
        for(int i=0; i<NUM_INDIRECT_LVL; i++)
        {
            level_blocks *= superblock.block_size / sizeof(u32);
            if(rem_blocks < level_blocks)
            {
                ans = allocate_indirect(&inode_s.pointers[NUM_DIRECT_PTR+i],
                                        i+1, rem_blocks);
                break;
            }
            rem_blocks -= level_blocks;
        }
    }

    if(ans == 0) // Block couldn't be allocated
        return 0;

    // Block was allocated
    inode_s.num_blocks++;
    if(inode_s.type == FILETYPE_DIRECTORY || inode_s.type == FILETYPE_SYMLINK)
        inode_s.bytes_size += superblock.block_size;

    res = write_inode(inode, &inode_s);
    if(res != 0)
        return 0;

    return ans;
}


/*-----------------------------------------------------------------------------
Funct:  Deallocate the last 'count' blocks of the given inode.
        If count is -1, this function deallocates all blocks.
Input:  inode -> The inode that needs block deallocation
        count -> How many blocks to deallocate
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int deallocate_blocks(u32 inode, int count)
{
    if(count == 0)
        return 0;

    struct t2fs_inode inode_s;
    int res = read_inode(inode, &inode_s);
    if(res != 0)
        return 0;

    if(count == -1)
        count = inode_s.num_blocks;
    u32 counter = count;
    inode_s.num_blocks -= counter;

    for(int i=NUM_INODE_PTR-1; i>=0 && counter>0; i--)
    {
        int level = MAX(0, i-NUM_DIRECT_PTR+1);
        res = deallocate_indirect(&inode_s.pointers[i], level, &counter);
        if(res != 0)
            break;
    }

    inode_s.num_blocks += counter; // If some could not have been deallocated
    return write_inode(inode, &inode_s);
}


/*-----------------------------------------------------------------------------
Funct:  Increment the hard link counter of a given inode.
Input:  inode -> The inode whose hl_counter is to be incremented
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int inc_hl_count(u32 inode)
{
    struct t2fs_inode inode_s;
    int res = read_inode(inode, &inode_s);
    if(res != 0)
        return res;
    inode_s.hl_count++;
    return write_inode(inode, &inode_s);
}


/*-----------------------------------------------------------------------------
Funct:  Decrement the hard link counter of a given inode, deleting the inode
            if it reaches 0.
Input:  inode -> The inode whose hl_counter is to be decremented
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int dec_hl_count(u32 inode)
{
    struct t2fs_inode inode_s;
    int res = read_inode(inode, &inode_s);
    if(res != 0)
        return res;
    if(--inode_s.hl_count == 0)
    {
        deallocate_blocks(inode, -1);
        close_all_inode(inode); // To prevent reading garbage
        struct t2fs_inode aux = {};
        inode_s = aux;
        res = operate_bitmap(inode, true, 0); // Mark inode as free
    }
    return write_inode(inode, &inode_s);
}


/*-----------------------------------------------------------------------------
Funct:  Given an inode, apply a given function to each of its composing data
            blocks, in ascending order, according to the function's return:
        =0 : The function succeeded. Return success without iterating further;
        <0 : The function returned an error. Return error immediately;
        >0 : The function didn't succeed: iterate further.
        The function must return an int and have a block number type (u32) as
            its first argument, though it can have any number of additional
            arguments, which must be received in a single va_list variable.
        It's the caller's responsibility to ensure that the number of
            additional arguments provided to this function matches the number
            of arguments expected by the given function inside the va_list.
Input:  inode -> The given inode
        fn    -> The function that should be applied to each block
        ...   -> The additional arguments
Return: If, at any point, the given function succeeds when called with a block,
            0 is returned.
        If all calls return positive and there is no blocks left to apply, the
            return is a positive value.
        If an error occurs at any point, a negative value will be returned.
-----------------------------------------------------------------------------*/
int iterate_inode_blocks(u32 inode, int (*fn)(u32, va_list), ...)
{
    struct t2fs_inode inode_s;
    int res = read_inode(inode, &inode_s);
    if(res != 0)
        return res;
    va_list args;
    va_start(args, fn); // Initialize arguments after the last named one
    for(int i=0; i<NUM_INODE_PTR; i++)
    {
        res = 1;
        u32 block = inode_s.pointers[i];
        if(block == 0) // No blocks left to apply
            break;
        int level = MAX(0, i-NUM_DIRECT_PTR+1); // Levels of indirection
        va_list a;
        va_copy(a, args); // To be able to use it again
        res = iterate_indirection(block, level, fn, a);
        if(res <= 0)
            break;
    }
    va_end(args); // Every va_start needs a va_end
    return res;
}
