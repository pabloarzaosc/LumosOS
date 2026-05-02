

#include "disk.h"


#define ATA_PORT_DATA       0x1F0   
#define ATA_PORT_ERROR      0x1F1   
#define ATA_PORT_SEC_COUNT  0x1F2   
#define ATA_PORT_LBA_LOW    0x1F3   
#define ATA_PORT_LBA_MID    0x1F4   
#define ATA_PORT_LBA_HIGH   0x1F5   
#define ATA_PORT_DRIVE_HEAD 0x1F6   
#define ATA_PORT_STATUS     0x1F7   
#define ATA_PORT_COMMAND    0x1F7   


#define ATA_CMD_READ        0x20    
#define ATA_CMD_WRITE       0x30    
#define ATA_CMD_FLUSH       0xE7    


#define ATA_STATUS_BSY      0x80    
#define ATA_STATUS_DRDY     0x40    
#define ATA_STATUS_DRQ      0x08    
#define ATA_STATUS_ERR      0x01    





static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}


static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}


static inline unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}


static inline void outw(unsigned short port, unsigned short value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}




static void ata_wait_busy(void) {
    while (inb(ATA_PORT_STATUS) & ATA_STATUS_BSY) {
        
    }
}


static int ata_wait_drq(void) {
    while (1) {
        unsigned char status = inb(ATA_PORT_STATUS);

        if (status & ATA_STATUS_ERR) {
            return -1;  
        }

        if (status & ATA_STATUS_DRQ) {
            return 0;   
        }

        
    }
}


static void ata_send_command(int drive, unsigned int lba, unsigned char cmd) {
    
    ata_wait_busy();

    
    unsigned char drive_head = (drive == ATA_SLAVE) ? 0xF0 : 0xE0;
    drive_head |= (unsigned char)((lba >> 24) & 0x0F);

    
    outb(ATA_PORT_DRIVE_HEAD, drive_head);     
    outb(ATA_PORT_SEC_COUNT,  1);              
    outb(ATA_PORT_LBA_LOW,   (unsigned char)(lba & 0xFF));         
    outb(ATA_PORT_LBA_MID,   (unsigned char)((lba >> 8) & 0xFF)); 
    outb(ATA_PORT_LBA_HIGH,  (unsigned char)((lba >> 16) & 0xFF));

    
    outb(ATA_PORT_COMMAND, cmd);
}




int disk_read_sector(int drive, unsigned int lba, void *buf) {
    unsigned short *buffer = (unsigned short *)buf;

    
    ata_send_command(drive, lba, ATA_CMD_READ);

    
    if (ata_wait_drq() != 0) {
        return -1;  
    }

    
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_PORT_DATA);
    }

    return 0;  
}


int disk_write_sector(int drive, unsigned int lba, const void *buf) {
    const unsigned short *buffer = (const unsigned short *)buf;

    
    ata_send_command(drive, lba, ATA_CMD_WRITE);

    
    if (ata_wait_drq() != 0) {
        return -1;  
    }

    
    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, buffer[i]);
    }

    
    outb(ATA_PORT_COMMAND, ATA_CMD_FLUSH);
    ata_wait_busy();

    return 0;  
}


int disk_read(unsigned int sector, void *buf) {
    return disk_read_sector(ATA_SLAVE, sector, buf);
}

int disk_write(unsigned int sector, const void *buf) {
    return disk_write_sector(ATA_SLAVE, sector, buf);
}
