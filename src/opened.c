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

static int block_t2fs_rw(u32 block, va_list args)
{
    u32 *offset = va_arg(args, u32*);
    u32 *bytes_left = va_arg(args, u32*);
    byte_t **buffer = va_arg(args, byte_t**);

    (void)block; (void)offset; (void)bytes_left; (void)buffer;
    // TODO: Implement
    return -1;
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


// TODO: Define
int t2fs_read_data();
int t2fs_write_data();
