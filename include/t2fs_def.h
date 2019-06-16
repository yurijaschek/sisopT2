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

#ifndef T2FS_DEF_H
#define T2FS_DEF_H


/**************************
 *  Constant definitions  *
 **************************/

#define T2FS_FILENAME_MAX       32 // Max size for filename (including '\0')
#define T2FS_PATH_MAX         1024 // Max size for a path (including '\0')
#define T2FS_MAX_FILES_OPENED   10 // Max number of regular files opened

// Types of files in the file system
enum filetype
{
    FILETYPE_INVALID = 0,
    FILETYPE_REGULAR,
    FILETYPE_DIRECTORY,
    FILETYPE_SYMLINK,
};


#endif // T2FS_DEF_H
