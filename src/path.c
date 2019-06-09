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
    str++;
    while(len > 1) // And not (len > 0) to copy the '\0' when moving
    {
        if(*str != '/' || *(str-1) != '/')
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
    for(; *str; str++)
    {
        if(*str == '/')
        {
            *str = '\0';
            return str+1;
        }
    }
    return 0; // NULL
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

    int max_link = 128; // To avoid infinite loops with symlinks
start: // When a link is found
    if(path[0] == '\0') // Empty string not acceptable
        return ans;
    remove_repeated_slashes(path); // So ".//file" == "./file", for example
    if(strcmp(path, "/") == 0) // It's the root directory
    {
        ans.valid = ans.exists = true;
        ans.type = FILETYPE_DIRECTORY;
        strcpy(ans.name, "/");
        ans.inode = ans.par_inode = ROOT_INODE;
        return ans;
    }

    // To store the '/' positions in the path
    char *curr = path; // The current name in the path
    char *next; // To know if curr is the last name
    // The inode of the current directory in the search
    u32 dir_inode = cwd_inode; // Assume it's relative
    if(path[0] == '/') // It's absolute
    {
        curr++; // Ignore the initial '/'
        dir_inode = ROOT_INODE;
    }

    for(;;)
    {
        next = next_name(curr); // The first directory name now is '\0'-ended
        u32 inode = get_inode_by_name(curr, dir_inode); // Search for the file
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
                // TODO: path = contents(file) + "/" + path
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
            // TODO: path = contents(file)
            goto start;
        }
    }
}
