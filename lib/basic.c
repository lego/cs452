#include <basic.h>
#include <assert.h>

#ifdef DEBUG_MODE
void debugger() {
}
#endif


void *jmemcpy(void *destination, const void *source, size_t num) {
  char *csrc = (char *)source;
  char *cdest = (char *)destination;
  int i;

  for (i = 0; i < num; i++)
    cdest[i] = csrc[i];
  return destination;
}

void *jmemmove(void *destination, const void *source, size_t num) {
  char *csrc = (char *)source;
  char *cdest = (char *)destination;
  char temp[num];
  int i;

  for (i = 0; i < num; i++)
    temp[i] = csrc[i];

  for (i = 0; i < num; i++)
    cdest[i] = temp[i];

  return destination;
}


int c2d( char ch ) {
  if( ch >= '0' && ch <= '9' ) return ch - '0';
  if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
  if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
  return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
  int num, digit;
  char *p;

  p = *src; num = 0;
  while( ( digit = c2d( ch ) ) >= 0 ) {
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
