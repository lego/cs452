#pragma once

#include <basic.h>

unsigned int jstrlen(const char *c);

bool jstrcmp(const char *str1, const char *str2);
void jstrappend(char *str1, char *str2, char *buf);
void jstrappendc(char *str1, char c, char *buf);


// Count amount of substrings
// NOTE: internal
int jstrsplit_count(char *str, char delimiter);

char **jstrsplit(char *str, char delimiter);

// Returns status:
// 0 => OK
int jstrsplit_buf(char *str, char delimiter, char *buffer, int buffer_size);

int jc2i(char c);
int ja2i(char* str);
void jui2a(unsigned int num, unsigned int base, char *bf);
void ji2a(int num, char *bf);

// array to unsigned int, and also produces a status
unsigned int jatoui( char *str, int *status );


void jformat ( char *buf, int buf_size, char *fmt, va_list va );

// __attribute format does compile time checks for the format argument
// see format in https://gcc.gnu.org/onlinedocs/gcc-4.3.2/gcc/Function-Attributes.html#Function-Attributes
void jformatf(char *buf, int buf_size, char* fmt, ...) __attribute__ ((format (printf, 3, 4)));
