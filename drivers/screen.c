

#include "screen.h"



static volatile unsigned char *const VGA_MEMORY = (volatile unsigned char *)0xB8000;



static int cursor_row = 0;
static int cursor_col = 0;


static unsigned char current_color = 0x0F;





static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}


static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}



static void update_cursor(void) {
    unsigned short position = (unsigned short)(cursor_row * SCREEN_WIDTH + cursor_col);

    
    outb(0x3D4, 14);                         
    outb(0x3D5, (unsigned char)(position >> 8));  

    
    outb(0x3D4, 15);                         
    outb(0x3D5, (unsigned char)(position & 0xFF)); 
}




unsigned char make_color(unsigned char fg, unsigned char bg) {
    return (unsigned char)((bg << 4) | (fg & 0x0F));
}


void set_color(unsigned char color) {
    current_color = color;
}



void screen_scroll(void) {
    
    for (int row = 1; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            int src = (row * SCREEN_WIDTH + col) * 2;
            int dst = ((row - 1) * SCREEN_WIDTH + col) * 2;
            VGA_MEMORY[dst]     = VGA_MEMORY[src];       
            VGA_MEMORY[dst + 1] = VGA_MEMORY[src + 1];   
        }
    }

    
    int last_row = SCREEN_HEIGHT - 1;
    for (int col = 0; col < SCREEN_WIDTH; col++) {
        int offset = (last_row * SCREEN_WIDTH + col) * 2;
        VGA_MEMORY[offset]     = ' ';           
        VGA_MEMORY[offset + 1] = current_color;  
    }

    
    cursor_row = SCREEN_HEIGHT - 1;
}




void screen_init(void) {
    current_color = make_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    cursor_row = 0;
    cursor_col = 0;
    clear_screen();
}


void clear_screen(void) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        VGA_MEMORY[i * 2]     = ' ';           
        VGA_MEMORY[i * 2 + 1] = current_color;  
    }
    cursor_row = 0;
    cursor_col = 0;
    update_cursor();
}


void print_char(char c) {
    if (c == '\n') {
        
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\t') {
        
        cursor_col = (cursor_col + 4) & ~3;
        if (cursor_col >= SCREEN_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    } else {
        
        int offset = (cursor_row * SCREEN_WIDTH + cursor_col) * 2;
        VGA_MEMORY[offset]     = (unsigned char)c;    
        VGA_MEMORY[offset + 1] = current_color;       
        cursor_col++;

        
        if (cursor_col >= SCREEN_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    
    if (cursor_row >= SCREEN_HEIGHT) {
        screen_scroll();
    }

    update_cursor();
}


void print_string(const char *str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}


void print_string_color(const char *str, unsigned char color) {
    unsigned char old_color = current_color;
    current_color = color;
    print_string(str);
    current_color = old_color;
}


void screen_backspace(void) {
    if (cursor_col > 0) {
        cursor_col--;
    } else if (cursor_row > 0) {
        
        cursor_row--;
        cursor_col = SCREEN_WIDTH - 1;
    } else {
        
        return;
    }

    
    int offset = (cursor_row * SCREEN_WIDTH + cursor_col) * 2;
    VGA_MEMORY[offset]     = ' ';
    VGA_MEMORY[offset + 1] = current_color;

    update_cursor();
}
