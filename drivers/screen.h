

#ifndef SCREEN_H
#define SCREEN_H


#define SCREEN_WIDTH  80    
#define SCREEN_HEIGHT 25    



#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GRAY    0x7
#define COLOR_DARK_GRAY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF




void screen_init(void);


void clear_screen(void);


void print_char(char c);


void print_string(const char *str);


void print_string_color(const char *str, unsigned char color);


void set_color(unsigned char color);


unsigned char make_color(unsigned char fg, unsigned char bg);


void screen_backspace(void);


void screen_scroll(void);

#endif 
