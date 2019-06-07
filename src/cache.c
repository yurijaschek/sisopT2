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
 *   Disk cache management functions
 */

#include "apidisk.h"
#include "libt2fs.h"
#include <string.h>


/************************
 *  Internal variables  *
 ************************/

byte_t sector_buffer[SECTOR_SIZE];


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Read data from the given disk sector to the given data buffer.
Input:  data   -> Where to store the data read
        sector -> The given sector to be read, relative to the partition
        offset -> Offset in the sector where the desired data is to be read
        size   -> Number of bytes to be read
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int t2fs_read_sector(byte_t *data, u32 sector, int offset, int size)
{
    sector += superblock.first_sector;
    int res = read_sector(sector, sector_buffer);
    if(res != 0)
        return res;
    memcpy(data, sector_buffer + offset, size);
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Write the given data to the given disk sector.
Input:  data   -> Where the data is
        sector -> The given sector to be written, relative to the partition
        offset -> Offset in the sector where the desired data is to be written
        size   -> Number of bytes to be written
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int t2fs_write_sector(byte_t *data, u32 sector, int offset, int size)
{
    sector += superblock.first_sector;
    int res = read_sector(sector, sector_buffer);
    if(res != 0)
        return res;
    memcpy(sector_buffer + offset, data, size);
    res = write_sector(sector, sector_buffer);
    if(res != 0)
        return res;
    return 0;
}
