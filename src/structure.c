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
Funct:  Given a directory data block and its indirection level, apply a given
            function to each of its composing data blocks, in ascending order.
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
static int
iterate_indirection(u32 block, int level,
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
Funct:  Given a directory inode, apply a given function to each of its
            composing data blocks, in ascending order, according to the
            function's return value:
        =0 : The function succeeded. Return success without iterating further;
        <0 : The function returned an error. Return error immediately;
        >0 : The function didn't succeed: iterate further.
        The function must return an int and have a block number type (u32) as
            its first argument, though it can have any number of additional
            arguments, which must be received in a single va_list variable.
        It's the caller's responsibility to ensure that the number of
            additional arguments provided to this function matches the number
            of arguments expected by the given function inside the va_list.
Input:  dir_inode -> The given directory inode
        fn        -> The function that should be applied to each block
        ...       -> The additional arguments
Return: If, at any point, the given function succeeds when called with a block,
            0 is returned.
        If all calls return positive and there is no blocks left to apply, the
            return is a positive value.
        If an error occurs at any point, a negative value will be returned.
-----------------------------------------------------------------------------*/
static int iterate_dir_blocks(u32 dir_inode, int (*fn)(u32, va_list), ...)
{
    struct t2fs_inode dir;
    int res = read_inode(dir_inode, &dir);
    if(res != 0)
        return res;
    va_list args;
    va_start(args, fn); // Initialize arguments after the last named one
    for(int i=0; i<NUM_INODE_PTR; i++)
    {
        res = 1;
        u32 block = dir.pointers[i];
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

    byte_t *data = malloc(superblock.block_size);
    if(!data)
        return -1;
    int res = t2fs_read_block(data, block);
    if(res != 0)
        goto ret;
    struct t2fs_record *dir = (struct t2fs_record*)data;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    res = 1;
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode == 0) // Found unused entry
        {
            dir[i].inode = inode;
            strcpy(dir[i].name, name);
            res = t2fs_write_block(data, block);
            break;
        }
    }
ret:
    free(data);
    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Search entries in a specific directory block by name.
Input:  block -> Block in which to search for the entry
        args  -> va_list that must be composed of:
            name  -> Name of the file to be searched for
            inode -> Pointer to return the inode of the file, if found
Return: On success, 0 is returned. On error, a negative value is returned.
        Otherwise, if it's not in the block, a positive value is returned.
-----------------------------------------------------------------------------*/
static int block_get_inode_by_name(u32 block, va_list args)
{
    char *name = va_arg(args, char*);
    u32 *inode = va_arg(args, u32*);

    byte_t *data = malloc(superblock.block_size);
    if(!data)
        return -1;
    int res = t2fs_read_block(data, block);
    if(res != 0)
        goto ret;
    struct t2fs_record *dir = (struct t2fs_record*)data;
    int num_entries = superblock.block_size / sizeof(struct t2fs_record);
    res = 1;
    *inode = 0;
    for(int i=0; i<num_entries; i++)
    {
        if(dir[i].inode != 0 && strcmp(dir[i].name, name) == 0) // Found entry
        {
            *inode = dir[i].inode;
            res = 0;
            break;
        }
    }
ret:
    free(data);
    return res;
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
    // Apply the "block_insert_entry" function to each block of the directory
    int res = iterate_dir_blocks(dir_inode, block_insert_entry,
                                 name, inode);
    if(res <= 0) // Error or it was successful
        return res;
    // Couldn't insert entry. Allocate new block
    u32 block = allocate_new_block(dir_inode);
    if(block == 0)
        return -1;
    // Insert specifically in this block, since the previous ones are full
    return call_directly(block_insert_entry, block, name, inode);
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
    u32 inode = 0; // To gather the answer of block_get_inode_by_name
    // Apply the "block_get_inode_by_name" function to each directory block
    iterate_dir_blocks(dir_inode, block_get_inode_by_name,
                       name, &inode);
    // If the function failed, inode would still be 0
    return inode;
}
