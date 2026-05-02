

#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#define FS_MAGIC_0          'L'
#define FS_MAGIC_1          'M'
#define FS_MAGIC_2          'F'
#define FS_MAGIC_3          'S'

#define FS_MAX_FILES        64      
#define FS_MAX_FILENAME     16      
#define FS_TOTAL_SECTORS    256     

#define FS_SUPERBLOCK_SECTOR 0
#define FS_FAT_SECTOR        1
#define FS_ROOT_DIR_START    2
#define FS_ROOT_DIR_SECTORS  4      
#define FS_DATA_START        6      

#define FAT_FREE             0x0000
#define FAT_EOF              0xFFFF

#define FILE_TYPE_FREE       0
#define FILE_TYPE_FILE       1
#define FILE_TYPE_DIR        2




typedef struct {
    char           magic[4];       
    unsigned short version;        
    unsigned short total_sectors;  
    unsigned short fat_start;      
    unsigned short root_dir_start; 
    unsigned short data_start;     
    char           reserved[498];  
} __attribute__((packed)) Superblock;


typedef struct {
    char           name[FS_MAX_FILENAME]; 
    unsigned int   size;                  
    unsigned short start_cluster;         
    unsigned char  type;                  
    char           reserved[9];           
} __attribute__((packed)) FileEntry;




int fs_init(void);


int fs_format(void);


void fs_list(void);


int fs_create_file(const char *name);


int fs_create_dir(const char *name);


int fs_change_dir(const char *path);


const char *fs_get_current_path(void);


int fs_write_file(const char *name, const char *data, unsigned int size);


int fs_append_file(const char *name, const char *data, unsigned int size);


int fs_read_file(const char *name, char *buf, unsigned int *out_size);


int fs_delete_file(const char *name);


int fs_exists(const char *name);


void fs_disk_info(unsigned int *total, unsigned int *used, unsigned int *free_secs);

int fs_rename(const char *old_name, const char *new_name);
int fs_copy(const char *src_name, const char *dst_name);
int fs_get_size(const char *name);
int fs_find(const char *name);

#endif 
