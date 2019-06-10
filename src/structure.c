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

// find_unused_entry()


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Insert an entry in a directory.
đInput:  dir_inode -> Inode of the directory to which insert the entry
        name      -> Name of the file entry to be added
        inode     -> Inode of the file entry to be added
Return: On success, 0 is returned.
        Otherwise, if the directory is full, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int insert_entry(u32 dir_inode, char *name, u32 inode)
{
    struct t2fs_inode dir;
    int res = read_inode(dir_inode, &dir);
    if(res != 0)
        return res;
    // TODO: Finish it
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
    // TODO: Finish it
}
