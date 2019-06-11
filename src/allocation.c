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
Funct:  Find a new free block to use.
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


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Read the given inode from disk (inodes table) to memory.
Input:  inode -> The given inode to be read
        data  -> Where to store the inode information read
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
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
        return -1;

    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Write the given inode from memory to disk (inodes table).
Input:  inode -> The given inode to be written
        data  -> Where the inode information to be written is
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
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
        return -1;

    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Find a new free inode to use.
Input:  type -> Type of the file the inode corresponds to
Return: On success, returns the inode number (positive integer).
        If there are no inodes available, 0 is returned.
-----------------------------------------------------------------------------*/
u32 find_new_inode(u8 type)
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
Return: On success, returns the inode number (positive integer).
        If there are no inodes available, 0 is returned.
-----------------------------------------------------------------------------*/
u32 allocate_new_block(u32 inode)
{
    // TODO: Implement
    (void)inode;
    return 0;
}
