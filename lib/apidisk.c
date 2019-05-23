#include "apidisk.h"
#include <stdio.h>

static char filename[] = "t2fs_disk.dat";


int read_sector(unsigned int sector, unsigned char *buffer)
{
    FILE *fp = fopen(filename, "r");
    if(!fp)
        return -1;
    int res = fseek(fp, sector*SECTOR_SIZE, SEEK_SET);
    if(res == 0) // fseek successful
    {
        res = fread(buffer, SECTOR_SIZE, 1, fp); // fread() should return 1
        res--;
    }
    fclose(fp);
    return res; // Will be 0 if it's successfully read a block
}


int write_sector (unsigned int sector, unsigned char *buffer)
{
    FILE *fp = fopen(filename, "w");
    if(!fp)
        return -1;
    int res = fseek(fp, sector*SECTOR_SIZE, SEEK_SET);
    if(res == 0) // fseek successful
    {
        res = fwrite(buffer, SECTOR_SIZE, 1, fp); // fwrite() should return 1
        res--;
    }
    fclose(fp);
    return res;
}

