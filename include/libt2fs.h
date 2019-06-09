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

#ifndef LIBT2FS_H
#define LIBT2FS_H

#include "t2fs.h"
#include <stdbool.h>
#include <stdint.h>


/***********************
 *  Macro definitions  *
 ***********************/

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof((x)[0]))

#define CHK_BIT(x,i) ((x) &   (1<<(i)))
#define SET_BIT(x,i) ((x) |=  (1<<(i)))
#define CLR_BIT(x,i) ((x) &= ~(1<<(i)))


/***********************************
 *  Constant and type definitions  *
 ***********************************/

#define NUM_DIRECT_PTR 3 // Number of direct block pointers in inode
#define T2FS_SIGNATURE "os sisopeiros" // Magic string in the superblock
#define ROOT_INODE 1U

typedef uint8_t byte_t;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


/*********************
 *  Data structures  *
 *********************/

// Alignment is to be avoided for structures stored on disk
#pragma pack(push, 1)

// Partition table entry
struct t2fs_partition
{
    u32 first_sector; // First valid sector of the partition
    u32 last_sector;  // Last valid sector of the partition
    char name[24];    // Name of the partition
};

// Master Boot Record structure
struct t2fs_mbr
{
    u16 version;     // 0x7E31 (16*year + semester)
    u16 sector_size; // 0x0100 (256 bytes)
    u16 pt_offset;   // Partition table offset: 0x08 (8 bytes)
    u16 pt_entries;  // Partition table entries: 0x04 (4 partitions)
    struct t2fs_partition ptable[4]; // Partition table (with 4 entries)
};

// Superblock of our file system partition
struct t2fs_superblock
{
    u8   sectors_per_block; // Number of disk sectors per logical data block
    char signature[15]; // To be certain this partition was formatted by us
    u32  first_sector;  // First sector of the partition (where superblock is)
    u32  num_sectors;   // Number of disk sectors this partition has
    u32  num_blocks;    // Number of logical data blocks
    u32  num_inodes;    // Number of inodes
    u32  it_offset;     // Sector offset of the inodes table
    u32  ib_offset;     // Sector offset of the inodes bitmap
    u32  bb_offset;     // Sector offset of the blocks bitmap
    u32  blocks_offset; // Sector offset of the logical blocks
};

// Index node, which stores information about files
struct t2fs_inode
{
    u8  type;       // Type of the file (regular, directory etc)
    u32 hl_count;   // Number of hard links that have this inode
    u32 bytes_size; // Size of the file, in bytes
    u32 direct_ptr[NUM_DIRECT_PTR]; // Direct pointers to blocks
    u32 singly_ptr; // Singly indirect pointer
    u32 doubly_ptr; // Doubly indirect pointer
    u32 triply_ptr; // Triply indirect pointer
};

// Directory record (entry): what directories are composed of
struct t2fs_record
{
    char name[T2FS_FILENAME_MAX]; // Name of the file (unique to the directory)
    u32  inode; // inode with the file's information (0 means unused entry)
};

#pragma pack(pop)

// File descriptor (of opened files)
struct t2fs_descriptor
{
    s32 id;       // Descriptor identifier (FILE2 or DIR2) (0 = invalid)
    u32 curr_pos; // Byte offset from the beginning of the file
    u32 inode;    // The inode that corresponds to the opened file
};

// Path information for a file
struct t2fs_path
{
    bool valid;     // Indicates whether the path to the file is valid or not
    bool exists;    // Indicates whether the file already exists or not
    u8   type;      // The file type, if it exists, according to enum filetype
    char name[T2FS_FILENAME_MAX]; // Basename of the file, if it path is valid
    u32  par_inode; // Inode of parent directory of the file, if path is valid
    u32  inode;     // The inode of the file, if it exists
};


/************************************
 *  Declarations of open functions  *
 ************************************/

// allocation.c
int read_inode(u32 inode, struct t2fs_inode *data);
int write_inode(u32 inode, struct t2fs_inode *data);
u32 use_new_inode(u8 type);

// cache.c
int t2fs_read_sector(byte_t *buffer, u32 sector, int offset, int size);
int t2fs_write_sector(byte_t *buffer, u32 sector, int offset, int size);

// descriptor.c

// init.c
int init_format(int sectors_per_block, int partition);
int init_t2fs(int partition);

// path.c
struct t2fs_path get_path_info(char *filepath, bool resolve);

// structure.c

// t2fs.c
void print_superblock(struct t2fs_superblock *sblock);


/**************************************
 *  Shared variables between sources  *
 **************************************/

extern struct t2fs_superblock superblock; // To hold management information
extern u32 cwd_inode; // Inode number of the current working directory
extern char cwd_path[T2FS_PATH_MAX]; // String with the cwd

#endif // LIBT2FS_H
