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

#ifndef T2FS_H
#define T2FS_H

#include <stdint.h>


/***********************************
 *  Constant and type definitions  *
 ***********************************/

#define T2FS_FILENAME_MAX     32
#define T2FS_PATH_MAX       1024

// Types of files in the file system
enum filetype
{
    FILETYPE_INVALID = 0,
    FILETYPE_REGULAR,
    FILETYPE_DIRECTORY,
    FILETYPE_SYMLINK,
};

typedef int32_t FILE2; // For regular files handles
typedef int32_t DIR2;  // For directory handles

// Record that holds directory entry information of a file, read with readdir2
typedef struct
{
    char name[T2FS_FILENAME_MAX];   // Name of the file whose entry was read
    uint8_t  fileType;  // Type of the file, according to enum filetype
    uint32_t fileSize;  // Size of the file, in bytes
} DIRENT2;


/*************************************************
 *  API functions declaration and documentation  *
 *************************************************/

/*
 *   Some notes and conventions of expressions used:
 *
 * - filepath (or simply path) is any string that specifies a unique location
 *     in the file system.
 *   It can always be either relative or absolute for this T2FS version.
 *
 * - basename of a path is simply the file name, without any parent directory
 *     in it. Example: "t2fs.c" is the basename of "/sisopT2/src/t2fs.c".
 *
 * - dirname is the opposite of the basename: the path without the file name.
 *   Example: "." is the dirname of "t2fs.c" (because of relative path).
 *
 * - a path is valid if the dirname of the path exists and can be followed,
 *     the file existing or not. Otherwise, the path is invalid.
 *
 * - a symbolic link (or symlink, soft link, or simply link) is a type of file
 *     that contains the path to another file.
 *   Since hard links are implemented through inode sharing, the expression
 *     "link" will be used solely to refer to soft links in this context.
 *
 * - the meaning of "to resolve" a file or link can be thought of in two ways:
 *   - to determine some path to a file that does not contain links, or
 *   - to determine the file that is ultimately being pointed to by a link.
 *   Some functions resolve links and some don't, depending on circumstances.
 */

/*-----------------------------------------------------------------------------
Funct:  Copy the developers' identification to the given buffer.
Input:  name -> Buffer to copy the information to
        size -> Size of the buffer, not to be exceeded
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size);


/*-----------------------------------------------------------------------------
Funct:  Logically format the partition 0 of the virtual 't2fs_disk.dat' disk to
            be used with our T2FS file system, using data blocks size multiple
            of the sector size.
        't2fs_disk.dat' is expected to already have an MBR and a partition 0.

Input:  sectors_per_block -> Size of data block, in disk sectors

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block);


/*-----------------------------------------------------------------------------
Funct:  Create a new regular file, given its path.
        If the path is invalid, it's an error.
        This function also opens the file (see open2) and returns its handle.
        If a file with the same name already exists, that one will have its
            contents truncated to 0 and the file will be opened as normal.

Input:  path -> Path to the file to be created and opened

Return: On success, the file handle is returned (positive integer).
        Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Delete an existing regular or link file, given its path.
        If the file doesn't exist, it's an error.
        This function will delete a link, if given one, and not the file the
            link resolves to.

Input:  path -> Path to the file to be deleted

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int delete2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Open an existing regular file, given its path.
        The opened file can then be the target of other functions through the
            returned handle, namely: close2, read2, write2, truncate2, seek2.
        If given a link, the link is resolved to the actual file.
        If the file doesn't exist, or if a link has been given and it cannot be
            resolved to an existing regular file, it's an error.
        Once opened, the current position for operations on the file will be 0.

Input:  path -> Path to the file to be opened

Return: On success, the file handle is returned (positive integer).
        Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Close an opened regular file, given its handle.
        The closed file will no longer be able to be operated upon until it's
            opened again.
        If the handle is invalid, it's an error.

Input:  handle -> Identifier of the opened file to be closed

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle);


/*-----------------------------------------------------------------------------
Funct:  Read bytes from an opened regular file into a buffer.
        The current position on the file is adjusted to the following byte of
            the last byte read.
        If the handle is invalid, or if size is negative, it's an error.

Input:  handle -> Identifier of the opened file to be read from
        buffer -> Buffer to store the read data
        size   -> Number of bytes to be read

Return: On success, the number of bytes effectively read is returned.
        Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size);


/*-----------------------------------------------------------------------------
Funct:  Write bytes to an opened regular file from a buffer.
        The current position on the file is adjusted to the following byte of
            the last byte written.
        If the handle is invalid, or if size is negative, it's an error.

Input:  handle -> Identifier of the opened file to be written to
        buffer -> Buffer that stores the data to be written
        size   -> Number of bytes to be written

Return: On success, the number of bytes effectively written is returned.
        Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size);


/*-----------------------------------------------------------------------------
Funct:  Truncate an opened regular file to its current position.
        This function discards all bytes from (and including) the current
            position up to the end of file. The current position becomes, then,
            the end of file.
        If the handle is invalid, it's an error.

Input:  handle -> Identifier of the opened file to be truncated

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle);


/*-----------------------------------------------------------------------------
Funct:  Reposition the current position (CP) of an opened regular file.
        The position is counted from the beginning of the file, in bytes, and
            it cannot surpass the size of the file.
        The only negative value accepted is -1, which corresponds to EOF.
        If the handle is invalid, it's an error.

Input:  handle -> Identifier of the opened file to have its CP repositioned
        offset -> New position for CP

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, uint32_t offset);


/*-----------------------------------------------------------------------------
Funct:  Create a new directory, given its path.
        If a file with the same name already exists, or the path is invalid,
            it is an error.

Input:  filename -> Path to the directory to be created

Return: On success, the file handle is returned (positive integer).
        Otherwise, a negative value is returned.

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int mkdir2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Delete an existing directory, given its path.
        If the directory doesn't exist, or if it's not empty, it's an error.
        This function only accepts directories. The user cannot delete a
            directory through a link.

Input:  path -> Path to the directory to be deleted

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int rmdir2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Change the current working directory (CWD) to the given path.
        If the path can't be resolved to an existing directory, it's an error.

Input:  path -> The path to the new CWD

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int chdir2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Copy the absolute path of the CWD to the given buffer.

Input:  path -> Buffer to copy the information to
        size -> Size of the buffer, not to be exceeded

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int getcwd2 (char *path, int size);


/*-----------------------------------------------------------------------------
Funct:  Open an existing directory, given its path.
        The opened directory can then be the target of other functions through
            the returned handle, namely: readdir2, closedir2.
        If given a link, the link is resolved to the actual file.
        If the directory doesn't exist, or if a link has been given and it
            cannot be resolved to an existing directory, it's an error.
        Once opened, the current entry for readdir2 operations on the directory
            will be the entry 0.

Input:  path -> Path to the directory to be opened

Return: On success, the directory handle is returned (positive integer).
        Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *path);


/*-----------------------------------------------------------------------------
Funct:  Fill the directory entry structure with information of the next valid
            entry in the directory, given its handle.
        To read all directory entries, this function must be called multiple
            times, until there is no entry left, at which point the entry
            position will always stay at EOF and an error will be returned.
        If the handle is invalid or if there are no more entries to be read,
            it's an error.

Input:  handle -> Identifier of the opened directory to read entries from
        dentry -> Directory entry structure to be filled

Return: On success, 0 is returned.
        Otherwise, a non-zero value is returned and dentry will be invalid.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry);


/*-----------------------------------------------------------------------------
Funct:  Close an opened directory, given its handle.
        The closed directory will no longer be able to be operated upon until
            it's opened again.
        If the handle is invalid, it's an error.

Input:  handle -> Identifier of the opened directory to be closed

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle);


/*-----------------------------------------------------------------------------
Funct:  Create a symbolic link file, given its path and what it points to.
        This function does not verify if the file it points to exists, or even
            if it's a valid path. It's the user's responsibility.
        This function only creates the given 'linkpath' file, if it can, and
            copies the given 'pointpath' string as its contents.
        If the given linkpath is invalid, or if linkpath exists and it's a
            regular or directory file, it's an error. If it's a link, then its
            contents will be overwritten by the new 'pointpath' provided.

Input:  linkpath  -> Path of where the link should be created
        pointpath -> Path that the link should point to

Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int ln2 (char *linkpath, char *pointpath);


#endif // T2FS_H
