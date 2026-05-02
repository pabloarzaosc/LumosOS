

#include "screen.h"
#include "keyboard.h"
#include "string.h"
#include "disk.h"
#include "filesystem.h"


#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length = 0;

#define HISTORY_MAX 10
static char history[HISTORY_MAX][INPUT_BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;

static unsigned int uptime_ticks = 0;

#define MAX_FILE_SIZE 8192
static char file_buffer[MAX_FILE_SIZE];


static unsigned char color_prompt, color_input, color_output, color_error, color_accent, color_banner, color_dim, color_success;


static void init_colors(void);
static void print_banner(void);
static void print_prompt(void);
static void print_error(const char* msg);
static void process_command(char *cmd_line);

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_echo(char *args);
static void cmd_about(void);
static void cmd_reboot(void);

static void cmd_ls(void);
static void cmd_cd(char *args);
static void cmd_pwd(void);
static void cmd_mkdir(char *args);
static void cmd_rmdir(char *args);
static void cmd_cat(char *args);
static void cmd_touch(char *args);
static void cmd_write(char *args);
static void cmd_append(char *args);
static void cmd_rm(char *args);
static void cmd_exists(char *args);

static void cmd_diskinfo(void);
static void cmd_meminfo(void);

static void cmd_mv(char *args);
static void cmd_cp(char *args);
static void cmd_size(char *args);
static void cmd_find(char *args);
static void cmd_uptime(void);
static void cmd_cpuinfo(void);
static void cmd_sysinfo(void);
static void cmd_whoami(void);
static void cmd_version(void);
static void cmd_date(void);
static void cmd_history(void);


static inline void outb_local(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline unsigned char inb_local(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}


void kernel_main(void) {
    init_colors();
    screen_init();

    print_banner();
    fs_init();
    print_prompt();

    while (1) {
        char c = keyboard_getchar();
        uptime_ticks++; 

        if (c == KEY_ENTER) {
            print_char('\n');
            input_buffer[input_length] = '\0';

            if (input_length > 0) {
                
                for (int i = HISTORY_MAX - 1; i > 0; i--) {
                    str_copy_max(history[i], history[i-1], INPUT_BUFFER_SIZE);
                }
                str_copy_max(history[0], input_buffer, INPUT_BUFFER_SIZE);
                if (history_count < HISTORY_MAX) history_count++;
                
                process_command(input_buffer);
            }

            input_length = 0;
            history_index = -1;
            print_prompt();

        } else if (c == KEY_UP) {
            if (history_count > 0 && history_index < history_count - 1) {
                history_index++;
                while (input_length > 0) { screen_backspace(); input_length--; }
                str_copy_max(input_buffer, history[history_index], INPUT_BUFFER_SIZE);
                input_length = str_length(input_buffer);
                set_color(color_input);
                print_string(input_buffer);
            }

        } else if (c == KEY_DOWN) {
            if (history_index >= 0) {
                history_index--;
                while (input_length > 0) { screen_backspace(); input_length--; }
                if (history_index >= 0) {
                    str_copy_max(input_buffer, history[history_index], INPUT_BUFFER_SIZE);
                    input_length = str_length(input_buffer);
                    set_color(color_input);
                    print_string(input_buffer);
                } else {
                    input_buffer[0] = '\0';
                    input_length = 0;
                }
            }

        } else if (c == KEY_BACKSPACE) {
            if (input_length > 0) {
                input_length--;
                screen_backspace();
            }

        } else if (c == KEY_CTRL_C) {
            print_string_color("^C\n", color_dim);
            input_buffer[0] = '\0';
            input_length = 0;
            history_index = -1;
            print_prompt();

        } else if (c != 0) {
            if (input_length < INPUT_BUFFER_SIZE - 1) {
                input_buffer[input_length] = c;
                input_length++;
                set_color(color_input);
                print_char(c);
            }
        }
    }
}


static void init_colors(void) {
    color_prompt  = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    color_input   = make_color(COLOR_WHITE, COLOR_BLACK);
    color_output  = make_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    color_error   = make_color(COLOR_LIGHT_RED, COLOR_BLACK);
    color_accent  = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    color_banner  = make_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    color_dim     = make_color(COLOR_DARK_GRAY, COLOR_BLACK);
    color_success = make_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
}

static void print_banner(void) {
    print_string_color("\n", color_banner);
    print_string_color("   _                              ___  ____\n", color_banner);
    print_string_color("  | |   _   _ _ __ ___   ___  ___|  \\/  | |___ \n", color_banner);
    print_string_color("  | |  | | | | '_ ` _ \\ / _ \\/ __| |\\/| |  __|\n", color_banner);
    print_string_color("  | |__| |_| | | | | | | (_) \\__ \\ |  | | |___\n", color_banner);
    print_string_color("  |_____\\__,_|_| |_| |_|\\___/|___/_|  |_|_____|\n", color_banner);
    print_string_color("\n  ", color_dim);
    for (int i = 0; i < 50; i++) print_char('-');
    print_char('\n');
}

static void print_prompt(void) {
    print_string_color("LumosOS:", color_accent);
    print_string_color(fs_get_current_path(), color_accent);
    print_string_color(" > ", color_prompt);
    set_color(color_input);
}

static void print_error(const char* msg) {
    print_string_color("  [ERROR] ", color_error);
    print_string_color(msg, color_error);
    print_char('\n');
}

static void print_uint(unsigned int num, unsigned char color) {
    if (num == 0) {
        char zero[2] = {'0', '\0'};
        print_string_color(zero, color);
        return;
    }
    char buf[12];
    int pos = 0;
    while (num > 0) {
        buf[pos++] = '0' + (char)(num % 10);
        num /= 10;
    }
    for (int j = pos - 1; j >= 0; j--) {
        char digit[2] = {buf[j], '\0'};
        print_string_color(digit, color);
    }
}


static void process_command(char *cmd_line) {
    char *cmd = cmd_line;
    char *args = str_split_first_space(cmd_line);

    if (str_compare(cmd, "help") == 0) cmd_help();
    else if (str_compare(cmd, "clear") == 0) cmd_clear();
    else if (str_compare(cmd, "echo") == 0) cmd_echo(args);
    else if (str_compare(cmd, "about") == 0) cmd_about();
    else if (str_compare(cmd, "reboot") == 0) cmd_reboot();
    
    else if (str_compare(cmd, "ls") == 0) cmd_ls();
    else if (str_compare(cmd, "cd") == 0) cmd_cd(args);
    else if (str_compare(cmd, "pwd") == 0) cmd_pwd();
    else if (str_compare(cmd, "mkdir") == 0) cmd_mkdir(args);
    else if (str_compare(cmd, "rmdir") == 0) cmd_rmdir(args);
    else if (str_compare(cmd, "cat") == 0) cmd_cat(args);
    else if (str_compare(cmd, "touch") == 0) cmd_touch(args);
    else if (str_compare(cmd, "write") == 0) cmd_write(args);
    else if (str_compare(cmd, "append") == 0) cmd_append(args);
    else if (str_compare(cmd, "rm") == 0) cmd_rm(args);
    else if (str_compare(cmd, "exists") == 0) cmd_exists(args);
    
    else if (str_compare(cmd, "diskinfo") == 0) cmd_diskinfo();
    else if (str_compare(cmd, "meminfo") == 0) cmd_meminfo();
    
    else if (str_compare(cmd, "mv") == 0) cmd_mv(args);
    else if (str_compare(cmd, "cp") == 0) cmd_cp(args);
    else if (str_compare(cmd, "size") == 0) cmd_size(args);
    else if (str_compare(cmd, "find") == 0) cmd_find(args);
    else if (str_compare(cmd, "uptime") == 0) cmd_uptime();
    else if (str_compare(cmd, "cpuinfo") == 0) cmd_cpuinfo();
    else if (str_compare(cmd, "sysinfo") == 0) cmd_sysinfo();
    else if (str_compare(cmd, "whoami") == 0) cmd_whoami();
    else if (str_compare(cmd, "version") == 0) cmd_version();
    else if (str_compare(cmd, "date") == 0) cmd_date();
    else if (str_compare(cmd, "history") == 0) cmd_history();
    
    else {
        print_error("Command not found");
        if (str_starts_with("help", cmd)) print_string_color("  Did you mean: help?\n", color_dim);
        else if (str_starts_with("clear", cmd)) print_string_color("  Did you mean: clear?\n", color_dim);
        else if (str_starts_with("ls", cmd)) print_string_color("  Did you mean: ls?\n", color_dim);
    }
}



static void print_help_page(int page) {
    clear_screen();
    if (page == 1) {
        print_string_color("\n  LumosOS Help - Page 1/3 (System & Nav)\n", color_accent);
        print_string_color("  Use -> and <- to navigate. Press ESC to exit.\n", color_dim);
        print_string_color("\n  SYSTEM:\n", color_accent);
        print_string_color("    help      - list all commands\n", color_output);
        print_string_color("    clear     - clear screen\n", color_output);
        print_string_color("    history   - show command history\n", color_output);
        print_string_color("    reboot    - restart system\n", color_output);
        print_string_color("    echo <tx> - print text\n", color_output);
        print_string_color("    whoami    - show current user\n", color_output);

        print_string_color("\n  DIRECTORIES & NAVIGATION:\n", color_accent);
        print_string_color("    ls             - list files\n", color_output);
        print_string_color("    cd <dir>       - change directory\n", color_output);
        print_string_color("    pwd            - show current path\n", color_output);
        print_string_color("    mkdir <dir>    - create directory\n", color_output);
        print_string_color("    rmdir <dir>    - remove directory\n", color_output);
    } else if (page == 2) {
        print_string_color("\n  LumosOS Help - Page 2/3 (File Management)\n", color_accent);
        print_string_color("  Use -> and <- to navigate. Press ESC to exit.\n", color_dim);
        print_string_color("\n  FILES:\n", color_accent);
        print_string_color("    cat <file>     - print contents\n", color_output);
        print_string_color("    touch <file>   - create empty file\n", color_output);
        print_string_color("    write <f> <tx> - write data (overwrite)\n", color_output);
        print_string_color("    append <f> <tx>- append data\n", color_output);
        print_string_color("    rm <file>      - delete file\n", color_output);
        print_string_color("    mv <src> <dst> - move/rename file\n", color_output);
        print_string_color("    cp <src> <dst> - copy file\n", color_output);
        print_string_color("    size <file>    - show file size\n", color_output);
        print_string_color("    find <name>    - search for file\n", color_output);
    } else {
        print_string_color("\n  LumosOS Help - Page 3/3 (System Info & Debug)\n", color_accent);
        print_string_color("  Use -> and <- to navigate. Press ESC to exit.\n", color_dim);
        print_string_color("\n  SYSTEM INFO:\n", color_accent);
        print_string_color("    sysinfo   - system summary\n", color_output);
        print_string_color("    cpuinfo   - cpu details\n", color_output);
        print_string_color("    uptime    - system uptime\n", color_output);
        print_string_color("    version   - os version\n", color_output);
        print_string_color("    date      - system date\n", color_output);

        print_string_color("\n  DEBUG:\n", color_accent);
        print_string_color("    diskinfo  - disk statistics\n", color_output);
        print_string_color("    meminfo   - memory information\n\n", color_output);
    }
}

static void cmd_help(void) {
    int page = 1;
    print_help_page(page);
    while (1) {
        char c = keyboard_getchar();
        if (c == KEY_RIGHT && page < 3) {
            page++;
            print_help_page(page);
        } else if (c == KEY_LEFT && page > 1) {
            page--;
            print_help_page(page);
        } else if (c == KEY_ESC || c == KEY_ENTER) {
            clear_screen();
            break;
        }
    }
}

static void cmd_clear(void) { clear_screen(); }
static void cmd_echo(char *args) {
    if (!args) { print_char('\n'); return; }
    print_string_color("  ", color_output);
    print_string_color(args, color_output);
    print_char('\n');
}

static void cmd_about(void) {
    print_string_color("\n  LumosOS v0.3", color_accent);
    print_string_color(" - A small UNIX-like OS\n", color_output);
    print_string_color("  * Subdirectory navigation & tracking\n", color_dim);
    print_string_color("  * Command history (Up/Down arrows)\n", color_dim);
    print_string_color("  * FAT-like persistent filesystem\n\n", color_dim);
}

static void cmd_reboot(void) {
    print_string_color("  Rebooting...\n", color_dim);
    unsigned char good = 0x02;
    while (good & 0x02) { good = inb_local(0x64); }
    outb_local(0x64, 0xFE);
    while (1) { __asm__ volatile ("hlt"); }
}

static void cmd_ls(void) { fs_list(); }

static void cmd_cd(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    if (fs_change_dir(args) != 0) {
        print_error("Directory not found or invalid");
    }
}

static void cmd_pwd(void) {
    print_string_color("  ", color_output);
    print_string_color(fs_get_current_path(), color_output);
    print_char('\n');
}

static void cmd_mkdir(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    if (fs_create_dir(args) == 0) {
        print_string_color("  Directory created\n", color_success);
    } else {
        print_error("Already exists or disk full");
    }
}

static void cmd_rmdir(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    int res = fs_delete_file(args);
    if (res == 0) {
        print_string_color("  Directory removed\n", color_success);
    } else if (res == -2) {
        print_error("Directory not empty");
    } else {
        print_error("Directory not found");
    }
}

static void cmd_cat(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    unsigned int size = 0;
    if (fs_read_file(args, file_buffer, &size) != 0) {
        print_error("File not found or is a directory");
        return;
    }
    if (size == 0) {
        print_string_color("  (empty file)\n", color_dim);
        return;
    }
    print_string_color("  ", color_output);
    print_string_color(file_buffer, color_output);
    print_char('\n');
}

static void cmd_touch(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    if (fs_exists(args)) { print_error("Already exists"); return; }
    if (fs_create_file(args) == 0) {
        print_string_color("  File created\n", color_success);
    } else {
        print_error("Disk full");
    }
}

static void cmd_write(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    char *filename = args;
    char *content = str_split_first_space(args);
    if (!content) { print_error("Invalid usage"); return; }
    
    if (fs_write_file(filename, content, str_length(content)) == 0) {
        print_string_color("  Data written\n", color_success);
    } else {
        print_error("Disk full or invalid target");
    }
}

static void cmd_append(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    char *filename = args;
    char *content = str_split_first_space(args);
    if (!content) { print_error("Invalid usage"); return; }
    
    if (!fs_exists(filename)) { print_error("File not found"); return; }

    if (fs_append_file(filename, content, str_length(content)) == 0) {
        print_string_color("  Data appended\n", color_success);
    } else {
        print_error("Disk full or invalid target");
    }
}

static void cmd_rm(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    int res = fs_delete_file(args);
    if (res == 0) {
        print_string_color("  File deleted\n", color_success);
    } else if (res == -2) {
        print_error("Target is a directory (use rmdir)");
    } else {
        print_error("File not found");
    }
}

static void cmd_exists(char *args) {
    if (!args) { print_error("Invalid usage"); return; }
    if (fs_exists(args)) print_string_color("  Exists\n", color_success);
    else print_error("Not found");
}

static void cmd_diskinfo(void) {
    unsigned int total = 0, used = 0, free_secs = 0;
    fs_disk_info(&total, &used, &free_secs);
    print_string_color("\n  Disk Statistics:\n", color_accent);
    print_string_color("    Total Capacity: ", color_dim); print_uint(total * 512 / 1024, color_output); print_string_color(" KB\n", color_dim);
    print_string_color("    Used Space:     ", color_dim); print_uint(used * 512 / 1024, color_output); print_string_color(" KB\n", color_dim);
    print_string_color("    Free Space:     ", color_dim); print_uint(free_secs * 512 / 1024, color_success); print_string_color(" KB\n\n", color_dim);
}

static void cmd_meminfo(void) {
    print_string_color("\n  Memory Information:\n", color_accent);
    print_string_color("    Total RAM:     128 MB (Simulated)\n", color_dim);
    print_string_color("    Kernel Base:   0x1000\n", color_dim);
    print_string_color("    Stack Base:    0x90000\n", color_dim);
    print_string_color("    VGA Buffer:    0xB8000\n", color_dim);
    print_string_color("    Status:        Stable\n\n", color_dim);
}

static void cmd_mv(char *args) {
    if (!args) { print_error("Invalid usage: mv <src> <dst>"); return; }
    char *src = args;
    char *dst = str_split_first_space(args);
    if (!dst) { print_error("Invalid usage: mv <src> <dst>"); return; }
    int res = fs_rename(src, dst);
    if (res == 0) print_string_color("  Renamed successfully\n", color_success);
    else if (res == -2) print_error("Destination already exists");
    else print_error("Source file not found");
}

static void cmd_cp(char *args) {
    if (!args) { print_error("Invalid usage: cp <src> <dst>"); return; }
    char *src = args;
    char *dst = str_split_first_space(args);
    if (!dst) { print_error("Invalid usage: cp <src> <dst>"); return; }
    int res = fs_copy(src, dst);
    if (res == 0) print_string_color("  Copied successfully\n", color_success);
    else print_error("Copy failed (check source or disk space)");
}

static void cmd_size(char *args) {
    if (!args) { print_error("Invalid usage: size <file>"); return; }
    int s = fs_get_size(args);
    if (s < 0) print_error("File not found");
    else {
        print_string_color("  Size: ", color_dim);
        print_uint(s, color_output);
        print_string_color(" bytes\n", color_dim);
    }
}

static void cmd_find(char *args) {
    if (!args) { print_error("Invalid usage: find <name>"); return; }
    if (fs_find(args) == 0) print_string_color("  Found in current directory\n", color_success);
    else print_error("Not found");
}

static void cmd_uptime(void) {
    print_string_color("  System running for ", color_dim);
    print_uint(uptime_ticks / 10, color_output);
    print_string_color(" seconds\n", color_dim);
}

static void cmd_cpuinfo(void) {
    print_string_color("\n  CPU Information:\n", color_accent);
    print_string_color("    Model: x86 (Compatible)\n", color_output);
    print_string_color("    Cores: 1\n", color_output);
    print_string_color("    Mode:  32-bit Protected Mode\n\n", color_output);
}

static void cmd_sysinfo(void) {
    print_string_color("\n  LumosOS System Summary:\n", color_accent);
    print_string_color("    OS Version: v0.4 (Experimental)\n", color_output);
    cmd_uptime();
    unsigned int total = 0, used = 0, free_secs = 0;
    fs_disk_info(&total, &used, &free_secs);
    print_string_color("    Disk:       ", color_dim); print_uint(used * 512 / 1024, color_output); 
    print_string_color("/", color_dim); print_uint(total * 512 / 1024, color_output); print_string_color(" KB used\n\n", color_dim);
}

static void cmd_whoami(void) {
    print_string_color("  root\n", color_output);
}

static void cmd_version(void) {
    print_string_color("  LumosOS v0.4-stable\n", color_accent);
}

static void cmd_date(void) {
    print_string_color("  2026-05-02 18:30 (Simulated)\n", color_output);
}

static void cmd_history(void) {
    print_string_color("\n  Command History:\n", color_accent);
    for (int i = 0; i < history_count; i++) {
        print_uint(i + 1, color_dim);
        print_string_color(". ", color_dim);
        print_string_color(history[i], color_output);
        print_char('\n');
    }
    print_char('\n');
}
