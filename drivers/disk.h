

#ifndef DISK_H
#define DISK_H


#define SECTOR_SIZE 512    


#define ATA_MASTER 0       
#define ATA_SLAVE  1       




int disk_read_sector(int drive, unsigned int lba, void *buf);


int disk_write_sector(int drive, unsigned int lba, const void *buf);


int disk_read(unsigned int sector, void *buf);
int disk_write(unsigned int sector, const void *buf);

#endif 
