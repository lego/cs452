#pragma once

#include <basic.h>
#include <stdarg.h>

/**
 * Gets the length of a string, including null terminator
 */
unsigned int jstrlen(const char *str);

/**
 * Compares two strings against each other for equality
 * NOTE: this is different than strcmp, which returns an integer
 * specifying which string is great, or equal
 */
bool jstrcmp(const char *str1, const char *str2);

/**
 * Concats a two strings into a buffer.
 * NOTE: if str1 == buf, we just find the null terminate and append after that
 * there is no bounds checking on this, be careful!
 */
void jstrappend(const char *str1, const char *str2, char *buf);

/**
 * Concats a a string and character into a buffer.
 * NOTE: if str1 == buf, we just find the null terminate and append after that
 * there is no bounds checking on this, be careful!
 */
void jstrappendc(const char *str1, const char c, char *buf);

/**
 * Copies a string from src to dest, for up to n characters
 * NOTE: n includes the null terminator
 */
char * jstrncpy(char *dest, const char * src, int n);


/**
 * Counts the amount of substrings in a string by a given delimiter
 *  NOTE: internal function for jstrsplit_buf
 */
int jstrsplit_count(char *str, char delimiter);

/**
 * Splits the string with a delimiter into a buffer.
 * The format of this is the beginning of the string is a null terminated array
 * of offsets where the string is (substring count is length of offset array)
 *
 * The substrings themselves are null terminated at the provided offsets.
 *
 * For example "hi bye", split with space would become
 * 3,6,\0,hi,\0,bye,\0
 * (comma is next index, string is spread out chars)
 *
 * For example, hi would be a char * at &result[3], hence the first digit is 3
 *              bye would be a char * at &result[6], hence the second digit is 6
 */
// Returns status:
void jstrsplit_buf(char *str, char delimiter, char *buffer, int buffer_size);

/**
 * Decimal character to integer.
 * @param  c decimal character (e.g. '0'-'9')
 * @return   value of character
 *          If it's not a decimal number, -1
 */
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
