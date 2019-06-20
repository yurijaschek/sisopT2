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
#include <stdlib.h>
#include <string.h>


/************************
 *  Internal variables  *
 ************************/

static byte_t sector_buffer[SECTOR_SIZE];


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Read data from the given disk sector to the given data buffer.
Input:  data   -> Where to store the data read
        sector -> The given sector to be read, relative to the partition
        offset -> Offset in the sector where the desired data is to be read
        size   -> Number of bytes to read
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int t2fs_read_sector(byte_t *data, u32 sector, int offset, int size)
{
    if(sector >= superblock.num_sectors)
        return -1;
    if(offset + size > SECTOR_SIZE)
        return -1;
    sector += superblock.first_sector;
    int res = read_sector(sector, sector_buffer);
    if(res != 0)
        return -abs(res);
    memcpy(data, sector_buffer + offset, size);
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Write the given data to the given disk sector.
Input:  data   -> Where the data is
        sector -> The given sector to be written, relative to the partition
        offset -> Offset in the sector where the desired data is to be written
        size   -> Number of bytes to write
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int t2fs_write_sector(byte_t *data, u32 sector, int offset, int size)
{
    if(sector >= superblock.num_sectors)
        return -1;
    if(offset + size > SECTOR_SIZE)
        return -1;
    sector += superblock.first_sector;
    int res = read_sector(sector, sector_buffer);
    if(res != 0)
        return -abs(res);
    memcpy(sector_buffer + offset, data, size);
    res = write_sector(sector, sector_buffer);
    if(res != 0)
        return -abs(res);
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Read the given disk block to the given data buffer.
Input:  data  -> Where to store the data read
        block -> The given block to be read, relative to the partition
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int t2fs_read_block(byte_t *data, u32 block)
{
    if(block >= superblock.num_blocks)
        return -1;
    u32 sector = superblock.first_sector + superblock.blocks_offset
               + block * superblock.sectors_per_block;
    for(u8 i=0; i<superblock.sectors_per_block; i++)
    {
        int res = read_sector(sector+i, data);
        if(res != 0)
            return -abs(res);
        data += superblock.sector_size;
    }
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Write the given data to the given block.
Input:  data  -> Where the data is
        block -> The given block to be written, relative to the partition
Return: On success, 0 is returned. Otherwise, a negative value is returned.
-----------------------------------------------------------------------------*/
int t2fs_write_block(byte_t *data, u32 block)
{
    if(block >= superblock.num_blocks)
        return -1;
    u32 sector = superblock.first_sector + superblock.blocks_offset
               + block * superblock.sectors_per_block;
    for(u8 i=0; i<superblock.sectors_per_block; i++)
    {
        int res = write_sector(sector+i, data);
        if(res != 0)
            return -abs(res);
        data += superblock.sector_size;
    }
    return 0;
}
