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
 *   Debugging functions
 */

#include "libt2fs.h"

#include <stdio.h>


/************************
 *  External functions  *
 ************************/

void print_superblock(struct t2fs_superblock *sblock)
{
    printf("Superblock:\n");
    printf("\tsectors_per_block : %u\n", sblock->sectors_per_block);
    printf("\tsignature         : %s\n", sblock->signature);
    printf("\tfirst_sector      : %u\n", sblock->first_sector);
    printf("\tnum_sectors       : %u\n", sblock->num_sectors);
    printf("\tnum_blocks        : %u\n", sblock->num_blocks);
    printf("\tnum_inodes        : %u\n", sblock->num_inodes);
    printf("\tit_offset         : %u\n", sblock->it_offset);
    printf("\tib_offset         : %u\n", sblock->ib_offset);
    printf("\tbb_offset         : %u\n", sblock->bb_offset);
    printf("\tblocks_offset     : %u\n", sblock->blocks_offset);
}

