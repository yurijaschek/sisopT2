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
 *   Directory structure functions
 */

#include "libt2fs.h"


/************************
 *  Internal functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Insert an entry in a specific directory block.
Input:  block -> Block to which insert the entry
        name  -> Name of the file entry to be added
        inode -> Inode of the file entry to be added
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if the block is full, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_insert_entry(u32 block, char *name, u32 inode)
{
    // TODO: Implement
    (void)block; (void)name; (void)inode;
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a specific directory block by name.
Input:  block -> Block in which to search for the entry
        name  -> Name of the file to be searched for
        inode -> Pointer to return the inode of the file, if found
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if it's not in the block, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_get_inode_by_name(u32 block, char *name, u32 *inode)
{
    // TODO: Implement
    (void)block; (void)name; (void)inode;
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Given a directory inode, apply a given function to each of its
            composing logical blocks, according to the function's return value:
        0  : The function succeeded. Return success without iterating further;
        <0 : The function returned an error. Return error immediately;
        >0 : The function needs to iterate further.
Input:  dir_inode ->
Return: -
-----------------------------------------------------------------------------*/
static int iterate_inode_blocks(u32 dir_inode, ...)
{

}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Insert an entry in a directory.
Input:  dir_inode -> Inode of the directory to which insert the entry
        name      -> Name of the file entry to be added
        inode     -> Inode of the file entry to be added
Return: On success, 0 is returned.
        Otherwise, if the directory is full, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int insert_entry(u32 dir_inode, char *name, u32 inode)
{
    int res = iterate_inode_blocks(dir_inode, block_insert_entry,
                                   2, name, inode);
    if(res <= 0)
        return res;
    u32 block = allocate_new_block(dir_inode);
    if(block == 0)
        return -1;
    return block_insert_entry(block, name, inode);
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a directory by name, returning its inode, if found.
Input:  dir_inode -> Inode of the directory to be searched
        name      -> Name of the file to search for
Return: On success, the inode of the file (entry) is returned.
        Otherwise, if the file doesn't exist in the directory, 0 is returned.
-----------------------------------------------------------------------------*/
u32 get_inode_by_name(u32 dir_inode, char *name)
{
    u32 inode = 0;
    iterate_inode_blocks(dir_inode, block_get_inode_by_name, 2, name, &inode);
    return inode;
}
