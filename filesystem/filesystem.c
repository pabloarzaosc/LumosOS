

#include "filesystem.h"
#include "disk.h"
#include "string.h"
#include "screen.h"


static Superblock     superblock;
static unsigned short fat[FS_TOTAL_SECTORS];
static FileEntry      root_dir[FS_MAX_FILES];


static unsigned short current_dir_cluster = 0; 
static char current_path[64] = "/";
static FileEntry current_dir_cache[FS_MAX_FILES];
static FileEntry *active_dir = root_dir;

static char sector_buf[SECTOR_SIZE];


static void mem_set(void *dst, unsigned char val, int n) {
    unsigned char *d = (unsigned char *)dst;
    for (int i = 0; i < n; i++) d[i] = val;
}

static void mem_copy(void *dst, const void *src, int n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (int i = 0; i < n; i++) d[i] = s[i];
}



static int save_active_dir(void) {
    if (current_dir_cluster == 0) {
        char *dir_ptr = (char *)root_dir;
        for (int i = 0; i < FS_ROOT_DIR_SECTORS; i++) {
            mem_copy(sector_buf, dir_ptr + (i * SECTOR_SIZE), SECTOR_SIZE);
            if (disk_write(FS_ROOT_DIR_START + i, sector_buf) != 0) return -1;
        }
    } else {
        unsigned int bytes_written = 0;
        unsigned short current = current_dir_cluster;
        char *dir_ptr = (char *)current_dir_cache;
        
        while (current != 0 && current != FAT_EOF && bytes_written < sizeof(current_dir_cache)) {
            unsigned int to_write = sizeof(current_dir_cache) - bytes_written;
            if (to_write > SECTOR_SIZE) to_write = SECTOR_SIZE;
            
            mem_set(sector_buf, 0, SECTOR_SIZE);
            mem_copy(sector_buf, dir_ptr + bytes_written, to_write);
            
            if (disk_write(current, sector_buf) != 0) return -1;
            
            bytes_written += to_write;
            current = fat[current];
        }
    }
    return 0;
}

static int save_fs(void) {
    mem_set(sector_buf, 0, SECTOR_SIZE);
    mem_copy(sector_buf, &superblock, sizeof(Superblock));
    if (disk_write(FS_SUPERBLOCK_SECTOR, sector_buf) != 0) return -1;

    mem_copy(sector_buf, fat, sizeof(fat));
    if (disk_write(FS_FAT_SECTOR, sector_buf) != 0) return -1;

    return save_active_dir();
}

static int load_fs(void) {
    if (disk_read(FS_FAT_SECTOR, sector_buf) != 0) return -1;
    mem_copy(fat, sector_buf, sizeof(fat));

    char *dir_ptr = (char *)root_dir;
    for (int i = 0; i < FS_ROOT_DIR_SECTORS; i++) {
        if (disk_read(FS_ROOT_DIR_START + i, sector_buf) != 0) return -1;
        mem_copy(dir_ptr + (i * SECTOR_SIZE), sector_buf, SECTOR_SIZE);
    }
    return 0;
}



static int find_file(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (active_dir[i].type != FILE_TYPE_FREE && str_compare(active_dir[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_dir_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (active_dir[i].type == FILE_TYPE_FREE) {
            return i;
        }
    }
    return -1;
}

static unsigned short allocate_cluster(void) {
    for (int i = FS_DATA_START; i < FS_TOTAL_SECTORS; i++) {
        if (fat[i] == FAT_FREE) {
            fat[i] = FAT_EOF;
            return (unsigned short)i;
        }
    }
    return 0; 
}

static void free_cluster_chain(unsigned short start_cluster) {
    unsigned short current = start_cluster;
    while (current != 0 && current != FAT_EOF && current < FS_TOTAL_SECTORS) {
        unsigned short next = fat[current];
        fat[current] = FAT_FREE;
        current = next;
    }
}



int fs_format(void) {
    superblock.magic[0]   = FS_MAGIC_0;
    superblock.magic[1]   = FS_MAGIC_1;
    superblock.magic[2]   = FS_MAGIC_2;
    superblock.magic[3]   = FS_MAGIC_3;
    superblock.version    = 2;
    superblock.total_sectors = FS_TOTAL_SECTORS;
    superblock.fat_start  = FS_FAT_SECTOR;
    superblock.root_dir_start = FS_ROOT_DIR_START;
    superblock.data_start = FS_DATA_START;
    mem_set(superblock.reserved, 0, sizeof(superblock.reserved));

    mem_set(fat, 0, sizeof(fat));
    for(int i = 0; i < FS_DATA_START; i++) fat[i] = FAT_EOF;

    mem_set(root_dir, 0, sizeof(root_dir));

    active_dir = root_dir;
    current_dir_cluster = 0;
    str_copy(current_path, "/");

    return save_fs();
}

int fs_init(void) {
    if (disk_read(FS_SUPERBLOCK_SECTOR, sector_buf) != 0) {
        return fs_format();
    }
    mem_copy(&superblock, sector_buf, sizeof(Superblock));

    if (superblock.magic[0] != FS_MAGIC_0 ||
        superblock.magic[1] != FS_MAGIC_1 ||
        superblock.magic[2] != FS_MAGIC_2 ||
        superblock.magic[3] != FS_MAGIC_3 ||
        superblock.version != 2) {
        return fs_format();
    }

    active_dir = root_dir;
    current_dir_cluster = 0;
    str_copy(current_path, "/");

    return load_fs();
}

const char *fs_get_current_path(void) {
    return current_path;
}

int fs_change_dir(const char *path) {
    if (str_compare(path, "/") == 0) {
        active_dir = root_dir;
        current_dir_cluster = 0;
        str_copy(current_path, "/");
        return 0;
    }

    if (current_dir_cluster != 0) {
        
        return -1;
    }

    int idx = find_file(path);
    if (idx < 0 || active_dir[idx].type != FILE_TYPE_DIR) return -1;

    current_dir_cluster = active_dir[idx].start_cluster;
    
    mem_set(current_dir_cache, 0, sizeof(current_dir_cache));
    unsigned int bytes_read = 0;
    unsigned short current = current_dir_cluster;
    char *dir_ptr = (char *)current_dir_cache;

    while (current != 0 && current != FAT_EOF && bytes_read < sizeof(current_dir_cache)) {
        if (disk_read(current, sector_buf) != 0) return -1;
        unsigned int to_copy = sizeof(current_dir_cache) - bytes_read;
        if (to_copy > SECTOR_SIZE) to_copy = SECTOR_SIZE;
        mem_copy(dir_ptr + bytes_read, sector_buf, to_copy);
        bytes_read += to_copy;
        current = fat[current];
    }

    active_dir = current_dir_cache;
    str_copy(current_path, "/");
    str_concat(current_path, path);
    return 0;
}

void fs_list(void) {
    unsigned char color_f = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    unsigned char color_d = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    unsigned char color_dim = make_color(COLOR_DARK_GRAY, COLOR_BLACK);
    unsigned char color_text = make_color(COLOR_LIGHT_GRAY, COLOR_BLACK);

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (active_dir[i].type != FILE_TYPE_FREE) count++;
    }

    if (count == 0) {
        print_string_color("  Directory is empty.\n", color_dim);
        return;
    }

    print_char('\n');
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (active_dir[i].type == FILE_TYPE_FREE) continue;

        if (active_dir[i].type == FILE_TYPE_DIR) {
            print_string_color("  [D] ", color_d);
            print_string_color(active_dir[i].name, color_text);
        } else {
            print_string_color("  [F] ", color_f);
            print_string_color(active_dir[i].name, color_text);
            print_string_color(" (size: ", color_dim);
            
            unsigned int s = active_dir[i].size;
            if (s == 0) print_char('0');
            else {
                char buf[12]; int p = 0;
                unsigned int temp = s;
                while(temp > 0) { buf[p++] = '0' + (temp % 10); temp /= 10; }
                while(p > 0) print_char(buf[--p]);
            }
            print_string_color(")", color_dim);
        }
        print_char('\n');
    }
    print_char('\n');
}

int fs_exists(const char *name) {
    return (find_file(name) >= 0) ? 1 : 0;
}

int fs_create_file(const char *name) {
    if (str_length(name) >= FS_MAX_FILENAME) return -1;
    if (fs_exists(name)) return -1;

    int idx = find_free_dir_slot();
    if (idx < 0) return -1;

    str_copy(active_dir[idx].name, name);
    active_dir[idx].size = 0;
    active_dir[idx].start_cluster = 0;
    active_dir[idx].type = FILE_TYPE_FILE;

    return save_fs();
}

int fs_create_dir(const char *name) {
    if (str_length(name) >= FS_MAX_FILENAME) return -1;
    if (fs_exists(name)) return -1;

    if (current_dir_cluster != 0) {
        
        return -1;
    }

    int idx = find_free_dir_slot();
    if (idx < 0) return -1;

    unsigned short first_cluster = allocate_cluster();
    if (first_cluster == 0) return -1;

    
    unsigned short current = first_cluster;
    for (int i = 1; i < FS_ROOT_DIR_SECTORS; i++) {
        unsigned short next = allocate_cluster();
        if (next == 0) {
            free_cluster_chain(first_cluster);
            return -1;
        }
        fat[current] = next;
        current = next;
    }

    
    mem_set(sector_buf, 0, SECTOR_SIZE);
    current = first_cluster;
    while (current != 0 && current != FAT_EOF) {
        disk_write(current, sector_buf);
        current = fat[current];
    }

    str_copy(active_dir[idx].name, name);
    active_dir[idx].size = 0;
    active_dir[idx].start_cluster = first_cluster;
    active_dir[idx].type = FILE_TYPE_DIR;

    return save_fs();
}

int fs_delete_file(const char *name) {
    int idx = find_file(name);
    if (idx < 0) return -1;

    if (active_dir[idx].type == FILE_TYPE_DIR) {
        
        unsigned short current = active_dir[idx].start_cluster;
        FileEntry temp_dir[FS_MAX_FILES];
        mem_set(temp_dir, 0, sizeof(temp_dir));
        unsigned int bytes_read = 0;
        char *dir_ptr = (char *)temp_dir;

        while (current != 0 && current != FAT_EOF && bytes_read < sizeof(temp_dir)) {
            if (disk_read(current, sector_buf) != 0) return -1;
            unsigned int to_copy = sizeof(temp_dir) - bytes_read;
            if (to_copy > SECTOR_SIZE) to_copy = SECTOR_SIZE;
            mem_copy(dir_ptr + bytes_read, sector_buf, to_copy);
            bytes_read += to_copy;
            current = fat[current];
        }

        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (temp_dir[i].type != FILE_TYPE_FREE) return -2; 
        }
    }

    free_cluster_chain(active_dir[idx].start_cluster);
    active_dir[idx].type = FILE_TYPE_FREE;

    return save_fs();
}

static int write_chain(unsigned short start_cluster, const char *data, unsigned int size) {
    unsigned int bytes_written = 0;
    unsigned short current = start_cluster;

    while (bytes_written < size) {
        unsigned int to_write = size - bytes_written;
        if (to_write > SECTOR_SIZE) to_write = SECTOR_SIZE;

        mem_set(sector_buf, 0, SECTOR_SIZE);
        mem_copy(sector_buf, data + bytes_written, to_write);

        if (disk_write(current, sector_buf) != 0) return -1;

        bytes_written += to_write;

        if (bytes_written < size) {
            if (fat[current] == FAT_EOF) {
                unsigned short next = allocate_cluster();
                if (next == 0) return -1;
                fat[current] = next;
            }
            current = fat[current];
        }
    }
    return 0;
}

int fs_write_file(const char *name, const char *data, unsigned int size) {
    if (!fs_exists(name)) {
        if (fs_create_file(name) != 0) return -1;
    }
    int idx = find_file(name);
    if (idx < 0) return -1;
    if (active_dir[idx].type == FILE_TYPE_DIR) return -1;

    free_cluster_chain(active_dir[idx].start_cluster);
    active_dir[idx].start_cluster = 0;
    active_dir[idx].size = 0;

    if (size == 0) return save_fs();

    unsigned short first_cluster = allocate_cluster();
    if (first_cluster == 0) return -1;

    active_dir[idx].start_cluster = first_cluster;
    active_dir[idx].size = size;

    if (write_chain(first_cluster, data, size) != 0) return -1;

    return save_fs();
}

int fs_append_file(const char *name, const char *data, unsigned int add_size) {
    int idx = find_file(name);
    if (idx < 0) return -1;
    if (active_dir[idx].type == FILE_TYPE_DIR) return -1;

    unsigned int old_size = active_dir[idx].size;
    if (add_size == 0) return 0;

    if (old_size == 0) {
        return fs_write_file(name, data, add_size);
    }

    unsigned short current = active_dir[idx].start_cluster;
    while (fat[current] != FAT_EOF && fat[current] != FAT_FREE) {
        current = fat[current];
    }

    unsigned int offset_in_last = old_size % SECTOR_SIZE;
    unsigned int bytes_written = 0;

    if (offset_in_last > 0) {
        if (disk_read(current, sector_buf) != 0) return -1;
        
        unsigned int to_write = SECTOR_SIZE - offset_in_last;
        if (to_write > add_size) to_write = add_size;

        mem_copy(sector_buf + offset_in_last, data, to_write);
        if (disk_write(current, sector_buf) != 0) return -1;

        bytes_written += to_write;
    }

    if (bytes_written < add_size) {
        unsigned short next = allocate_cluster();
        if (next == 0) return -1;
        fat[current] = next;

        if (write_chain(next, data + bytes_written, add_size - bytes_written) != 0) return -1;
    }

    active_dir[idx].size += add_size;
    return save_fs();
}

int fs_read_file(const char *name, char *buf, unsigned int *out_size) {
    int idx = find_file(name);
    if (idx < 0) return -1;
    if (active_dir[idx].type == FILE_TYPE_DIR) return -1;

    unsigned int size = active_dir[idx].size;
    if (out_size) *out_size = size;
    if (size == 0) {
        buf[0] = '\0';
        return 0;
    }

    unsigned int bytes_read = 0;
    unsigned short current = active_dir[idx].start_cluster;

    while (current != 0 && current != FAT_EOF && bytes_read < size) {
        if (disk_read(current, sector_buf) != 0) return -1;

        unsigned int to_copy = size - bytes_read;
        if (to_copy > SECTOR_SIZE) to_copy = SECTOR_SIZE;

        mem_copy(buf + bytes_read, sector_buf, to_copy);
        bytes_read += to_copy;

        current = fat[current];
    }

    buf[size] = '\0';
    return 0;
}

void fs_disk_info(unsigned int *total, unsigned int *used, unsigned int *free_secs) {
    if (total) *total = FS_TOTAL_SECTORS;
    if (used || free_secs) {
        unsigned int u = FS_DATA_START; 
        for (int i = FS_DATA_START; i < FS_TOTAL_SECTORS; i++) {
            if (fat[i] != FAT_FREE) u++;
        }
        if (used) *used = u;
        if (free_secs) *free_secs = FS_TOTAL_SECTORS - u;
    }
}

int fs_rename(const char *old_name, const char *new_name) {
    int idx = find_file(old_name);
    if (idx < 0) return -1;
    if (find_file(new_name) >= 0) return -2;
    str_copy(active_dir[idx].name, new_name);
    return save_fs();
}

int fs_get_size(const char *name) {
    int idx = find_file(name);
    if (idx < 0) return -1;
    return (int)active_dir[idx].size;
}

int fs_find(const char *name) {
    int idx = find_file(name);
    return (idx >= 0) ? 0 : -1;
}

int fs_copy(const char *src_name, const char *dst_name) {
    char *copy_buf = (char *)0x50000;
    unsigned int size = 0;
    if (fs_read_file(src_name, copy_buf, &size) != 0) return -1;
    if (fs_create_file(dst_name) != 0) return -2;
    if (fs_write_file(dst_name, copy_buf, size) != 0) return -3;
    return 0;
}
