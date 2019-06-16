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


/************************
 *  Internal variables  *
 ************************/

// Position 0 is reserved to directory descriptor
// Positions 1 to T2FS_MAX_FILES_OPENED is for regular files
static struct t2fs_descriptor table[1+T2FS_MAX_FILES_OPENED];


/************************
 *  Internal functions  *
 ************************/

/************************
 *  External functions  *
 ************************/
