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
 *   Initialization functions
 */

#include "apidisk.h"
#include "libt2fs.h"

#include <string.h>


/************************
 *  Internal variables  *
 ************************/

static struct t2fs_mbr mbr; // Structure to hold the MBR
static bool init_done; // If we can work with the partition already or not
static byte_t sector_buffer[SECTOR_SIZE]; // Auxiliary space to hold a sector


/************************
 *  Internal functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Initialize the MBR structure, reading it from "t2fs_disk.dat".
        After the function succeeds once, it will always return success.
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
static int init_mbr()
{
    int res;

    if(mbr.sector_size == SECTOR_SIZE) // If already initialized
        return 0;

    res = read_sector(0, sector_buffer); // MBR is in sector 0
    if(res != 0)
        return res;

    mbr = *((struct t2fs_mbr*)sector_buffer); // Copy the MBR to its structure
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Calculate the maximum number of logical blocks that can fit in a
            partition, together with its bitmap of appropriate size, by doing
            binary search in the answer.
Input:  sectors_per_block -> Number of disk sectors in a logical block
        sectors_available -> Number of sectors available in the partition
Return: The number of logical blocks that can fit in the partition.
-----------------------------------------------------------------------------*/
static u32 calculate_num_blocks(u32 sectors_per_block, u32 sectors_available)
{
    u32 low = 1, high = sectors_available, mid, val;
    while(high - low > 1U)
    {
        mid = low + (high - low)/2;
        // Number of disk sectors, if the partition has mid blocks
        val = 1 + (mid-1)/(8*SECTOR_SIZE) + sectors_per_block*mid;
        if(val <= sectors_available)
            low = mid;
        else
            high = mid;
    }
    mid = high;
    val = 1 + (mid-1)/(8*SECTOR_SIZE) + sectors_per_block*mid;
    if(val > sectors_available)
        mid--;
    return mid;
}


/*-----------------------------------------------------------------------------
Funct:  Setup structures for the file system on the disk.
        The given superblock must be already initialized and valid.
        The structures offsets will be those in the superblock.
        The file system will only have the root ('/') directory created, with
            inode 1, and using the first available block (1). (0 means unused).
Input:  sblock -> Pointer to the superblock with the partition information
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
static int setup_structures(struct t2fs_superblock *sblock)
{
    print_superblock(sblock);
    int res = 0 ;

    memset(sector_buffer, 0, SECTOR_SIZE); // Clean sector for table and bitmaps
    u32 first = sblock->first_sector + sblock->it_offset;
    u32 last  = sblock->first_sector + sblock->blocks_offset; // Not included

    for(u32 i = first; i < last; ++i)
    {
        res = write_sector(i, sector_buffer);
        if(res != 0)
            return res;
    }

    memcpy(sector_buffer, sblock, sizeof(*sblock));
    res = write_sector(sblock->first_sector, sector_buffer);
    if(res != 0)
        return res;

    // TODO: Create the root ('/') directory at inode 1

    return 0;
}


/************************
 *  External functions  *
 ************************/

/*-----------------------------------------------------------------------------
Funct:  Format the specified partition of "t2fs_disk.dat".
        It reserves space for each of the internal structures needed, namely:
            (a) superblock;
            (b) inodes table;
            (c) inodes bitmap; and
            (d) blocks bitmap.
        All the necessary information is stored in the partition's superblock,
            and the internal structures are initialized accordingly.
        The number of inodes, which DOES NOT CHANGE unless the partition is
            formatted again, will be defined in terms of sectors reserved for
            the inodes table, which will be roughly 1% of the total number of
            sectors in the partition.
        The partition must have a size of at least 4 sectors + 1 logical block.
        After formatting, init_t2fs must be called at some point before any
            internal function.
Input:  sectors_per_block -> Number of disk sectors in a logical block,
                             between 1 and 128 (inclusive)
        partition         -> Which partition to be formatted
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int init_format(int sectors_per_block, int partition)
{
    int res;

    res = init_mbr(); // Make sure MBR is initialized
    if(res != 0)
        return res;

    if(partition >= mbr.pt_entries) // Invalid partition to format
        return -1;

    u32 first_sector = mbr.ptable[partition].first_sector;
    u32 last_sector  = mbr.ptable[partition].last_sector;

    if(first_sector > last_sector)
        return -1;

    u32 num_sectors = last_sector - first_sector + 1;
    if(num_sectors < (u32)sectors_per_block + 4) // Minimum number of sectors
        return -1;

    u32 remaining = num_sectors - 1; // Remaining sectors. Superblock reserved

    u32 it_sectors = remaining / 100; // Sectors for inodes table, roughly 1%
    if(it_sectors == 0)
        it_sectors++; // At least 1
    remaining -= it_sectors;

    u32 num_inodes = it_sectors * (SECTOR_SIZE / sizeof(struct t2fs_inode));
    // Sectors for inodes bitmap
    u32 ib_sectors = 1 + num_inodes / (8*SECTOR_SIZE); // Inodes counting start at 1
    remaining -= ib_sectors;

    u32 num_blocks = calculate_num_blocks(sectors_per_block, remaining);
    // Sectors for blocks bitmap
    u32 bb_sectors = 1 + num_blocks / (8*SECTOR_SIZE); // Blocks counting start at 1

    struct t2fs_superblock sblock =
    {
        .sectors_per_block = sectors_per_block,
        .signature = T2FS_SIGNATURE,
        .first_sector = first_sector,
        .num_sectors = num_sectors,
        .num_blocks = num_blocks,
        .num_inodes = num_inodes,
        .it_offset = 1U,
        .ib_offset = 1U + it_sectors,
        .bb_offset = 1U + it_sectors + ib_sectors,
        .blocks_offset = 1U + it_sectors + ib_sectors + bb_sectors,
    };

    res = setup_structures(&sblock);
    if(res != 0)
        return res;

    init_done = false;
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Check if the T2FS partition superblock is valid for us to work with.
        This function is also used to initialize the superblock structure.
        Unless the partition is formatted, subsequent calls to this function
            after a success will always return with success.
Input:  partition -> Which partition to be initialized
Return: On success, 0 is returned. Otherwise, a non-zero value is returned.
-----------------------------------------------------------------------------*/
int init_t2fs(int partition)
{
    int res;

    if(init_done) // Already initialized
        return 0;

    res = init_mbr(); // Make sure MBR is initialized
    if(res != 0)
        return res;

    if(partition >= mbr.pt_entries) // Invalid partition to initialize
        return -1;

    res = read_sector(mbr.ptable[partition].first_sector, sector_buffer); // Read superblock sector
    if(res != 0)
        return res;

    superblock = *((struct t2fs_superblock*)sector_buffer); // Copy the superblock
    if(strcmp(superblock.signature, T2FS_SIGNATURE) != 0) // Make sure it's our "magic string"
        return -1;

    init_done = true;
    return 0;
}

