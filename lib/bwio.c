#include <basic.h>
#include <util.h>
#include <bwio.h>
#include <jstring.h>

int bwputmc( int channel, char c , int n ) {
  int result = 0;
  while (n > 0 && result == 0) result = bwputc(channel, c);
  return result;
}

int bwputx( int channel, char c ) {
  char chh, chl;

  chh = c2x( c / 16 );
  chl = c2x( c % 16 );
  bwputc( channel, chh );
  return bwputc( channel, chl );
}

int bwputr( int channel, unsigned int reg ) {
  int byte;
  char *ch = (char *) &reg;

  for( byte = 3; byte >= 0; byte-- ) bwputx( channel, ch[byte] );
  return bwputc( channel, ' ' );
}

int bwputstr( int channel, char *str ) {
  while( *str ) {
    if( bwputc( channel, *str ) < 0 ) return -1;
    str++;
  }
  return 0;
}

void bwputw( int channel, int n, char fc, char *bf ) {
  char ch;
  char *p = bf;

  while( *p++ && n > 0 ) n--;
  while( n-- > 0 ) bwputc( channel, fc );
  while( ( ch = *bf++ ) ) bwputc( channel, ch );
}


// put a long int
void bwputl( int channel, long int n, char fc, char *bf ) {
  char ch;
  char *p = bf;

  while( *p++ && n > 0 ) n--;
  while( n-- > 0 ) bwputc( channel, fc );
  while( ( ch = *bf++ ) ) bwputc( channel, ch );
}

void bwprintf( int channel, char *fmt, ... ) {
  char buf[2048];
  va_list va;
  va_start(va, fmt);
  jformat(buf, 2048, fmt, va);
  va_end(va);
  bwputstr(channel, buf);
}
