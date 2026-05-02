

#include "keyboard.h"



static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}


#define KEYBOARD_DATA_PORT   0x60   
#define KEYBOARD_STATUS_PORT 0x64   



static const char scancode_to_ascii[128] = {
      0,   27,   '1', '2', '3', '4', '5', '6',
     '7',  '8',  '9', '0', '-', '=', '\b', '\t',
     'q',  'w',  'e', 'r', 't', 'y', 'u', 'i',
     'o',  'p',  '[', ']', '\n',  0,  'a', 's',
     'd',  'f',  'g', 'h', 'j', 'k', 'l', ';',
     '\'', '`',   0, '\\', 'z', 'x', 'c', 'v',
     'b',  'n',  'm', ',', '.', '/',  0,  '*',
      0,   ' ',   0,   0,   0,   0,   0,   0,
      0,    0,    0,   0,   0,   0,   0,   0,
      1,    0,   '-',  4,   0,   5,  '+',  0, 
      2,    0,    0,   0,   0,   0,   0,   0, 
      0,    0,    0,   0,   0,   0,   0,   0,
      0,    0,    0,   0,   0,   0,   0,   0,
      0,    0,    0,   0,   0,   0,   0,   0,
      0,    0,    0,   0,   0,   0,   0,   0,
      0,    0,    0,   0,   0,   0,   0,   0
};


static int ctrl_pressed = 0;


char keyboard_getchar(void) {
    unsigned char scancode;
    char ascii;

    while (1) {
        while ((inb(KEYBOARD_STATUS_PORT) & 0x01) == 0) {
            
        }

        scancode = inb(KEYBOARD_DATA_PORT);

        
        if (scancode == 0x1D) {
            ctrl_pressed = 1;
            continue;
        }
        if (scancode == 0x9D) {
            ctrl_pressed = 0;
            continue;
        }

        
        if (scancode & 0x80) {
            continue;
        }

        ascii = scancode_to_ascii[scancode];

        
        if (ctrl_pressed && (ascii == 'c' || ascii == 'C')) {
            return KEY_CTRL_C;
        }

        if (ascii != 0) {
            return ascii;
        }
    }
}
