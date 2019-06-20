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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


/************************
 *  Internal functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Given a directory function, that takes a block and a va_list as
            arguments, call the function directly.
        This function exists because va_list can't be "made up", so a variadic
            function must exist between if the given function needs a call.
        It's the caller's responsibility to ensure that the number of
            additional arguments provided to this function matches the number
            of arguments expected by the given function inside the va_list.
Input:  fn    -> The function that should be called directly
        block -> Block to be iterated
        ...   -> The additional arguments, to be translated to va_list
Return: The given function return is returned.
-----------------------------------------------------------------------------*/
static int call_directly(int (*fn)(u32, va_list), u32 block, ...)
{
    va_list args;
    va_start(args, block); // Initialize arguments after last named one
    int res = fn(block, args); // Call the function
    va_end(args); // Every va_start needs a va_end
    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Insert an entry in a specific directory block.
Input:  block -> Block to which insert the entry
        args  -> va_list that must be composed of:
            name  -> Name of the file entry to be added
            inode -> Inode of the file entry to be added
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if the block is full, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_insert_entry(u32 block, va_list args)
{
    char *name = va_arg(args, char*);
    u32 inode = va_arg(args, u32);

    int res = t2fs_read_block(block_buffer, block);
    if(res != 0)
        return res;
    struct t2fs_record *dir = (struct t2fs_record*)block_buffer;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode == 0) // Found unused entry
        {
            dir[i].inode = inode;
            strcpy(dir[i].name, name);
            return t2fs_write_block(block_buffer, block);
        }
    }
    return 1; // Iterate further
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a specific directory block by name, deleting it if
            asked to.
Input:  block -> Block in which to search for the entry
        args  -> va_list that must be composed of:
            name  -> Name of the file to be searched for
            inode -> Pointer to return the inode of the file, if found
            del   -> If the entry is to be deleted or not
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if it's not in the block, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_search_by_name(u32 block, va_list args)
{
    char *name = va_arg(args, char*);
    u32 *inode = va_arg(args, u32*);
    bool del = va_arg(args, int); // Should be int instead of bool
                            // because arguments smaller than int get promoted

    int res = t2fs_read_block(block_buffer, block);
    if(res != 0)
        return res;
    struct t2fs_record *dir = (struct t2fs_record*)block_buffer;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    *inode = 0;
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode != 0 && strcmp(dir[i].name, name) == 0) // Found entry
        {
            *inode = dir[i].inode;
            res = 0;
            if(del)
            {
                struct t2fs_record aux = {};
                dir[i] = aux;
                res = t2fs_write_block(block_buffer, block);
            }
            return res;
        }
    }
    return 1; // Iterate further
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a specific directory block by inode.
Input:  block -> Block in which to search for the entry
        args  -> va_list that must be composed of:
            name  -> Name of the file to be returned
            inode -> The inode to be searched for
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if it's not in the block, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_search_by_inode(u32 block, va_list args)
{
    char *name = va_arg(args, char*);
    u32 inode = va_arg(args, u32);

    int res = t2fs_read_block(block_buffer, block);
    if(res != 0)
        return res;
    struct t2fs_record *dir = (struct t2fs_record*)block_buffer;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode == inode) // Found entry
        {
            strcpy(name, dir[i].name);
            return 0;
        }
    }
    return 1; // Iterate further
}


/*-----------------------------------------------------------------------------
Funct:  Test if directory is deletable, given a specific block.
Input:  block -> Block of the directory to test whether it's deletable or not
        args  -> va_list that must be empty.
Return: On failure, 0 is returned. On error, a negative value is returned.
        Otherwise, if it can be deleted, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_dir_deletable(u32 block, va_list args)
{
    (void)args; // Unused parameter
    int res = t2fs_read_block(block_buffer, block);
    if(res != 0)
        return res;
    struct t2fs_record *dir = (struct t2fs_record*)block_buffer;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode != 0) // Valid entry, must be equal to "." or ".." only
        {
            if(strcmp(dir[i].name, ".") != 0 && strcmp(dir[i].name, "..") != 0)
                return 0; // Valid non-trivial entry, dir can't be deleted
        }
    }
    return 1; // Iterate further. Or, if at the last block, dir is deletable
}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Search entries in a directory by name, returning its inode, if found.
Input:  dir_inode -> Inode of the directory to be searched
        name      -> Name of the file to search for
Return: On success, the inode of the file (entry) is returned.
        Otherwise, if the file doesn't exist in the directory, 0 is returned.
-----------------------------------------------------------------------------*/
u32 get_inode_by_name(u32 dir_inode, char *name)
{
    u32 inode = 0; // To gather the answer of block_get_inode_by_name
    // Apply the "block_search_by_name" function to each directory block
    iterate_inode_blocks(dir_inode, block_search_by_name,
                         name, &inode, false);
    // If the function failed, inode would still be 0
    return inode;
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a directory by name, returning its inode, if found.
Input:  dir_inode -> Inode of the directory to be searched
        name      -> Name of the file to search for
Return: On success, the inode of the file (entry) is returned.
        Otherwise, if the file doesn't exist in the directory, 0 is returned.
-----------------------------------------------------------------------------*/
int get_name_by_inode(u32 dir_inode, char *name, u32 inode)
{
    // Apply the "block_search_by_inode" function to each directory block
    return iterate_inode_blocks(dir_inode, block_search_by_inode,
                                name, inode);
}


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
    // Apply the "block_insert_entry" function to each block of the directory
    int res = iterate_inode_blocks(dir_inode, block_insert_entry,
                                   name, inode);
    if(res < 0)
        return res;
    // Couldn't insert entry. Allocate new block
    if(res > 0)
    {
        u32 block = allocate_new_block(dir_inode);
        if(block == 0)
            return -1;
        // Insert specifically in this block, since previous ones are full
        res = call_directly(block_insert_entry, block, name, inode);
    }

    if(res == 0) // Success
    {
        if(inc_hl_count(inode) != 0)
        {
            delete_entry(dir_inode, name);
            res = -1;
        }
    }
    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Delete an entry in a directory.
Input:  dir_inode -> Inode of the directory in which delete the entry
        name      -> Name of the file entry to be deleted
Return: On success, 0 is returned.
        Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int delete_entry(u32 dir_inode, char *name)
{
    u32 inode = 0; // To gather the answer of block_get_inode_by_name
    // Apply the "block_search_by_name" function to each directory block
    iterate_inode_blocks(dir_inode, block_search_by_name,
                         name, &inode, true);

    int res = 0;
    if(inode != 0) // Not using the inode from this entry anymore
        res = dec_hl_count(inode);

    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Test if a directory can be deleted, given its inode.
Input:  dir_inode -> Inode of the directory to be tested
Return: Whether the direcory can be deleted (true) or not (false).
-----------------------------------------------------------------------------*/
bool dir_deletable(u32 dir_inode)
{
    if(iterate_inode_blocks(dir_inode, block_dir_deletable) > 0)
        return true;
    return false;
}
