

#ifndef STRING_H
#define STRING_H


int str_compare(const char *s1, const char *s2);


int str_length(const char *str);


void str_copy(char *dst, const char *src);


int str_starts_with(const char *str, const char *prefix);


const char *str_skip_prefix(const char *str, const char *prefix);


char *str_split_first_space(char *str);


void str_copy_max(char *dst, const char *src, int max_len);


void str_concat(char *dst, const char *src);

#endif 
