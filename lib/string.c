

#include "string.h"


int str_compare(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


int str_length(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}


void str_copy(char *dst, const char *src) {
    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';  
}


int str_starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str != *prefix) {
            return 0;  
        }
        str++;
        prefix++;
    }
    return 1;  
}


const char *str_skip_prefix(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str != *prefix) {
            return 0;  
        }
        str++;
        prefix++;
    }
    return str;  
}


char *str_split_first_space(char *str) {
    while (*str) {
        if (*str == ' ') {
            *str = '\0';
            str++;
            
            while (*str == ' ') {
                str++;
            }
            if (*str == '\0') {
                return 0; 
            }
            return str;
        }
        str++;
    }
    return 0;
}


void str_copy_max(char *dst, const char *src, int max_len) {
    if (max_len <= 0) return;
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}


void str_concat(char *dst, const char *src) {
    int i = 0;
    while (dst[i]) i++;
    int j = 0;
    while (src[j]) {
        dst[i] = src[j];
        i++;
        j++;
    }
    dst[i] = '\0';
}
