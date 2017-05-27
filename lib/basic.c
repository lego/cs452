#include <basic.h>

#ifdef DEBUG_MODE
void debugger() {
}
#endif


// See basic.h for why these are commented out
// void *memcpy(void *destination, const void *source, size_t num) {
//   jmemcpy(destination, source, num);
//   return destination;
// }
//
// void *memmove(void *destination, const void *source, size_t num) {
//   jmemmove(destination, source, num);
//   return destination;
// }

/**
 * This memcpy will copy over a word at a time, but this has crashing issues due to pre-alignment
 */
void jfastmemcpy(void *dest, const void *src, size_t num) {
  // log_debug("num=%d num/4=%d vsrc=%x vdest=%x", num, num / 4, src, dest);
  char *cdest = (char *) dest;
  char *csrc = (char *) src;
  int i;

  // int z = (unsigned int) csrc % 4;
  // if (z != 0) {
  //   for (i = 0; i < z; i++) {
  //     log_debug("i=%d csrc=%x cdest=%x", i, csrc + i, cdest + i);
  //     cdest[i] = csrc[i];
  //   }
  // }
  //
  // cdest += z;
  // csrc += z;
  unsigned int *vdest = (unsigned int *) cdest;
  unsigned int *vsrc = (unsigned int *) csrc;

  // log_debug("num=%d num/4=%d vsrc=%x vdest=%x", num, num / 4, vsrc, vdest);

  for (i = 0; i < num / 4; i++) {
    // log_debug("i=%d vsrc=%x vdest=%x", i, vsrc + i, vdest + i);
    vdest[i] = vsrc[i];
  }
  for (i = i * 4; i < num; i++) {
    // log_debug("i=%d csrc=%x cdest=%x", i, csrc + i, cdest + i);
    cdest[i] = csrc[i];
  }
  return;
}

/**
 * This memcpy will copy over a multiple words at a time, but also has some issues
 */
void jasmmemcpy(unsigned int *dest, const unsigned int *src, size_t num) {
  #ifdef DEBUG_MODE
  char *cdest = (char *) dest;
  char *csrc = (char *) src;
  int i;
  for (i = 0; i < num; i++)
    cdest[i] = csrc[i];
  #else
  // reserved registers for memory transfer
  asm volatile("stmfd sp!, {r3-r10}\n\t");
  // copy 8 at a time
  while (num > 8) {
    asm volatile (
      "ldmia r1!, {r3-r10} \n\t"
      "stmia r0!, {r3-r10} \n\t"
    );
    num -= 8;
  }
  asm volatile("ldmfd sp!, {r3-r10}\n\t");

  // copy remainder of 8
  char *cdest = (char *) dest;
  char *csrc = (char *) src;
  int i;
  for (i = 0; i < num / 4; i++)
    dest[i] = src[i];
  i *= 4;
  for (; i < num / 4; i++)
    cdest[i] = csrc[i];
  #endif
}

void jmemcpy(void *dest, const void *src, size_t num) {
  char *cdest = (char *) dest;
  char *csrc = (char *) src;
  int i;
  for (i = 0; i < num; i++) {
    cdest[i] = csrc[i];
  }
  return;
}

void jmemmove(void *destination, const void *source, size_t num) {
  char *csrc = (char *)source;
  char *cdest = (char *)destination;
  char temp[num];
  int i;

  for (i = 0; i < num; i++)
    temp[i] = csrc[i];

  for (i = 0; i < num; i++)
    cdest[i] = temp[i];
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

void ul2a( unsigned long int num, unsigned int base, char *bf ) {
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


void l2a( long int num, char *bf ) {
  if( num < 0 ) {
    num = -num;
    *bf++ = '-';
  }
  ul2a( num, 10, bf );
}


char c2x( char ch ) {
  assert(ch < 16);
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

unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}
