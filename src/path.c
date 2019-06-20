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
 *   File path functions
 */

#include "libt2fs.h"
#include <stdio.h>
#include <string.h>


/************************
 *  Internal functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Remove repeated (consecutive) slashes from a given string
Input:  str -> The given string
-----------------------------------------------------------------------------*/
static void remove_repeated_slashes(char *str)
{
    int len = strlen(str);
    str++; // Skip first character
    while(len > 1) // And not (len > 0) to copy the '\0' when moving
    {
        if(*str != '/' || *(str-1) != '/') // Test current and previous char
            str++;
        else // Consecutive slash
            memmove(str-1, str, len); // Overwrite the previous
        len--;
    }
}


/*-----------------------------------------------------------------------------
Funct:  Given a path string, find where the next name starts (hierarchy-wise).
        Please note that this function modifies the given string, writing a
            '\0' in the first occurrence of a '/', returning a pointer to the
            next byte after it.
Input:  str -> The given path string
Return: A pointer to the next name in the path string, just after the '\0' that
            was written over the first '/'.
        If there is no next name (no '/' in the string), NULL is returned.
-----------------------------------------------------------------------------*/
static char *next_name(char *str)
{
    for(; *str; str++) // Increments until '\0'
    {
        if(*str == '/')
        {
            *str = '\0';
            return str+1;
        }
    }
    return 0; // NULL
}


/*-----------------------------------------------------------------------------
Funct:  Given a path string, check if it ends with a slash and, if it does,
            append a '.' to it.
        In other words, if the path ends with a slash, it must be a directory.
Input:  str -> The given path string
-----------------------------------------------------------------------------*/
static void end_slash_force_dir(char *str)
{
    int len = strlen(str);
    if(str[len-1] == '/') // Ends with a slash
    {
        if(len+1 < T2FS_PATH_MAX)
        {
            str[len++] = '.';
            str[len] = '\0';
        }
    }
}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Given a path, get its information by following it.
        Symbolic links in the middle of a path (between '/') will always be
            followed, but if the path ends with a symbolic link, it will be
            followed only if 'resolve' is true.
Input:  path    -> The given path to (possibly) a file
        resolve -> If symbolic links in the end should be followed or not
Return: A structure containing information about the path is returned.
        For more information, check the structure definition.
-----------------------------------------------------------------------------*/
struct t2fs_path get_path_info(char *filepath, bool resolve)
{
    struct t2fs_path ans = {}; // To be returned
    char path[T2FS_PATH_MAX];  // To preserve the original string
    strncpy(path, filepath, T2FS_PATH_MAX);
    u32 dir_inode = cwd_inode; // Assume it's relative (will be checked later)

    int max_link = 128; // To avoid infinite loops with symlinks
start: // When a link is found
    if(path[0] == '\0') // Empty string not acceptable
        return ans;
    remove_repeated_slashes(path); // So ".//file" == "./file", for example
    end_slash_force_dir(path); // So "mydir/" == "mydir/.", for example
    // It also turns the root directory in "/."

    // To store the '/' positions in the path
    char *curr = path; // The current name in the path
    char *next; // To know if curr is the last name
    // The inode of the current directory in the search
    if(path[0] == '/') // It's absolute
    {
        curr++; // Ignore the initial '/'
        dir_inode = ROOT_INODE;
    }

    for(;;)
    {
        next = next_name(curr); // The first directory name now is '\0'-ended
        u32 inode = get_inode_by_name(dir_inode, curr); // Search for the file
        if(inode == 0) // File is not in the directory
        {
            if(next) // Invalid path
                return ans;
            // Valid path. File does not exist
            ans.valid = true;
            strcpy(ans.name, curr);
            ans.par_inode = dir_inode;
            return ans;
        }

        struct t2fs_inode file;
        if(read_inode(inode, &file) != 0) // Read file from inode
            return ans;

        if(next) // If there is another name in the path
        {
            if(file.type == FILETYPE_REGULAR) // Regular file can't have next
                return ans;
            else if(file.type == FILETYPE_DIRECTORY)
            {
                dir_inode = inode;
                curr = next;
            }
            else if(file.type == FILETYPE_SYMLINK)
            {
                if(max_link-- == 0)
                    return ans;
                if(t2fs_read_block(block_buffer, file.pointers[0]) != 0)
                    return ans;
                char aux[T2FS_PATH_MAX];
                strcpy(aux, next); // aux = next
                // path = contents(file) + "/" + next
                snprintf(path, MIN(T2FS_PATH_MAX, superblock.block_size),
                         "%s/%s", (char*)block_buffer, aux);
                goto start;
            }
        }
        else // next == NULL (curr is the last name in the path)
        if(file.type != FILETYPE_SYMLINK || !resolve) // File exists
        {
            ans.valid = ans.exists = true;
            ans.type = file.type;
            strcpy(ans.name, curr);
            ans.par_inode = dir_inode;
            ans.inode = inode;
            return ans;
        }
        else // Found a symlink file at the end and resolve == true
        {
            if(max_link-- == 0)
                return ans;
            if(t2fs_read_block(block_buffer, file.pointers[0]) != 0)
                return ans;
            // path = contents(file)
            strncpy(path, (char*)block_buffer,
                    MIN(T2FS_PATH_MAX, superblock.block_size));
            goto start;
        }
    }
}


/*-----------------------------------------------------------------------------
Funct:  Given a string, reverse it in place.
Input:  str -> The given string to be reversed
-----------------------------------------------------------------------------*/
void reverse_string(char *str)
{
    int i=0, len=strlen(str);
    for(i=0; i<len/2; i++)
    {
        char aux = str[i];
        str[i] = str[len-1-i];
        str[len-1-i] = aux;
    }
}
