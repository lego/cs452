/*
 * basic.c - basic helper functions
 *
 * Basic functions
 *
 */

#include <basic.h>

#ifdef DEBUG_MODE
void debugger() {
}
#endif

int a2d( char ch ) {
  if( ch >= '0' && ch <= '9' ) return ch - '0';
  if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
  if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
  return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
  int num, digit;
  char *p;

  p = *src; num = 0;
  while( ( digit = a2d( ch ) ) >= 0 ) {
    if ( digit > base ) break;
    num = num*base + digit;
    ch = *p++;
  }
  *src = p; *nump = num;
  return ch;
}


void ui2a( unsigned int num, unsigned int base, char *bf ) {
  int n = 0;
  int dgt;
  unsigned int d = 1;

  while( (num / d) >= base ) d *= base;
  while( d != 0 ) {
    dgt = num / d;
    num %= d;
    d /= base;
    if( n || dgt > 0 || d == 0 ) {
      *bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
      ++n;
    }
  }
  *bf = 0;
}

void i2a( int num, char *bf ) {
  if( num < 0 ) {
    num = -num;
    *bf++ = '-';
  }
  ui2a( num, 10, bf );
}

char c2x( char ch ) {
  #ifdef DEBUG_MODE
  assert(ch < 16);
  #endif
  if ( (ch <= 9) ) return '0' + ch;
  return 'a' + ch - 10;
}

bool is_digit( char ch ) {
  return '0' <= ch && ch <= '9';
}

bool is_alpha( char ch ) {
  return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z');
}

bool is_alphanumeric( char ch ) {
  return is_alpha(ch) || is_digit(ch);
}

unsigned int jatoui(char *str, int *status) {
  unsigned int res = 0;    // Initialize result
  unsigned int i = 0;    // Initialize index of first digit

  while (!is_digit(str[i]) && str[i] != '\0') i++;
  if (str[i] == '\0') {
    if (status != NULL) *status = -1;
    return 0;
  }

  // Iterate through all digits and update the result
  for (; is_digit(str[i]); ++i) {
    res = res * 10 + (str[i] - '0');
  }

  // Check if there are only spaces at the end
  while (str[i] == ' ') i++;
  // Status denoting we fully consumed only the number, excluding padding
  if (str[i] == '\0' && status != NULL) *status = 0;
  // Status denoting there were more non-number characters
  else if (status != NULL) *status = 1;
  return res;
}
