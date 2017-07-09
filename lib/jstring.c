#include <stddef.h>
#include <kassert.h>
#include <jstring.h>
#include <util.h>

unsigned int jstrlen(const char *c) {
  int len = 0;
  while(c[len] != 0) len++;
  return len;
}

bool jstrcmp(const char *str1, const char *str2) {
  int i = 0;
  while (true) {
    if (str1[i] == '\0' && str2[i] == '\0') return true;
    else if (str1[i] == str2[i]) i++;
    else return false;
  }
  return false;
}

void jstrappend(char *str1, char *str2, char *buf) {
  // FIXME: assert size doesn't overflow buffer
  if (buf == str1) {
    // optimization if str1 is also a buffer with space
    while (*buf != '\0') buf++;
  } else {
    while (*str1 != '\0') {
      *buf = *str1;
      buf++;
      str1++;
    }
  }
  while (*str2 != '\0') {
    *buf = *str2;
    buf++;
    str2++;
  }
  *buf = '\0';
}

void jstrappendc(char *str1, char c, char *buf) {
  // FIXME: assert size doesn't overflow buffer
  if (buf == str1) {
    // optimization if str1 is also a buffer
    while (*buf != '\0') buf++;
  } else {
    while (*str1 != '\0') {
      *buf = *str1;
      buf++;
      str1++;
    }
  }
  *buf = c;
  buf++;
  *buf = '\0';
}

char **jstrsplit(char *str, char delimiter) {
  // int i;
  // unsigned int len = strlen(str);
  //
  // // count for how many new strings there will be
  // int str_count = str[0] == delimiter ? 0 : 1;
  // bool in_delimiter = str[0] == delimiter;
  // for (i = 1; i < len; i++) {
  //   if (str[i] == delimiter && in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   str_count++;
  //   in_delimiter = str[i] == delimiter;
  // }
  //
  // char **strings = jalloc(sizeof(char **) * str_count);
  //
  // // count the length of each string
  // in_delimiter = str[0] == delimiter;
  // int curr_strlen = str[0] == delimiter ? 0 : 1;
  // for (i = 1; i < len; i++) {
  //   if (str[i] == delimiter && in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   if (str[i] != delimiter && !in_delimiter) continue;
  //   str_count++;
  //   in_delimiter = str[i] == delimiter;
  // }
  // return strings;
  return NULL;
}

int jstrsplit_count(char *str, char delimiter) {
  int i;
  unsigned int len = jstrlen(str);
  // edge case, empty string
  if (len == 0) return 0;

  // count for how many new strings there will be
  int str_count = str[0] == delimiter ? 0 : 1;
  bool in_delimiter = str[0] == delimiter;
  for (i = 0; i < len; i++) {
    if (str[i] != delimiter && in_delimiter) str_count++;
    in_delimiter = str[i] == delimiter;
  }

  return str_count;
}


int jstrsplit_buf(char *str, char delimiter, char *buffer, int buffer_size) {
  int i;
  unsigned int len = jstrlen(str);
  // FIXME: this will fail if the buffer is > 256, due to char size limits
  // TODO: assert buffer_size is enough
  // by worst case analysis, you need (strlen + 1) * 1.5 + 1 characters
  int str_count = jstrsplit_count(str, delimiter);

  // put fillers into counts
  for (i = 0; i < str_count; i++) {
    buffer[i] = 0x02;
  }
  // put NULL for char ** end
  buffer[str_count] = '\0';
  // edge case, empty string
  if (str_count == 0) return 0;

  // Set the split characters into the buffer, with offsets in the beginning
  char *current_str = buffer + str_count + 1;
  // start it in the delimiter so it always initializes
  bool in_delimiter = true;
  // start buffer back one, so it initializes with an increment correctly
  if (in_delimiter) buffer--;
  int idx = 0;
  for (i = 0; i < len; i++) {
    if (str[i] == delimiter && in_delimiter) continue;
    else if (str[i] != delimiter && in_delimiter) {
      // move out of delimiter
      // shift current char **
      buffer++;
      // set char **
      *buffer = idx + str_count + 1;
      *current_str = str[i];
    } else if (str[i] == delimiter && !in_delimiter) {
      // move into delimiter
      // add zero byte
      *current_str = '\0';
      debugger();
    } else if (str[i] != delimiter && !in_delimiter) {
      // in string
      // set string value
      *current_str = str[i];
    }
    current_str++;
    idx++;
    in_delimiter = str[i] == delimiter;
  }
  // add zero byte for last char, edge case if there is no final delim
  *current_str = '\0';

  return 0;
}

int jc2i(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  };
  return -1;
}

int ja2i(char* str) {
  int length;
  for (length = 0; *(str+length); length++);
  length -= 1; // go back to last character to skip the '\0'
  int total = 0;
  int base = 1;
  char c;
  for (; length >= 0; length--) {
    c = *(str+length);
    int converted = jc2i(c);
    if (converted < 0) {
      return -1;
    }
    total += converted * base;
    base *= 10;
  }
  return total;
}

void jui2a(unsigned int num, unsigned int base, char *bf) {
  int n = 0;
  int dgt;
  unsigned int d = 1;

  while((num / d) >= base) d *= base;
  while(d != 0) {
    dgt = num / d;
    num %= d;
    d /= base;
    if(n || dgt > 0 || d == 0) {
      *bf++ = dgt + (dgt < 10 ? '0' : 'a' - 10);
      ++n;
    }
  }
  *bf = 0;
}

void ji2a(int num, char *bf) {
  if(num < 0) {
    num = -num;
    *bf++ = '-';
  }
  jui2a(num, 10, bf);
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


// returns the final amount of characters appended
// equivilant to n+len(bf)
int jstrappendw( char *dest_buf, int n, char fc, char *bf ) {
  char ch;
  char *p = bf;
  int used_buf = 0;
  while( *p++ && n > 0 ) n--;
  while( n-- > 0 ) {
    jstrappendc( dest_buf, fc, dest_buf );
    used_buf++;
  }
  while( ( ch = *bf++ ) ) {
    jstrappendc( dest_buf, ch, dest_buf );
    used_buf++;
  }
  return used_buf;
}


void jformat ( char *buf, int buf_size, char *fmt, va_list va ) {
  buf[0] = '\0';
  char bf[12];
  char ch, lz;
  int w;
  int used_buf = 0;
  int used_size = 0;
  int trailing = 0;

  while ( ( ch = *(fmt++) ) ) {
    used_size = 0;
    if ( ch != '%' ) {
      jstrappendc( buf, ch, buf );
      used_buf++;
    } else {
      lz = 0; w = 0; trailing = 0;
      ch = *(fmt++);
      switch ( ch ) {
      case '0':
        lz = 1; ch = *(fmt++);
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        ch = a2i( ch, &fmt, 10, &w );
        if (lz == 1) lz = '0';
        else lz = ' ';
        break;
      case '-':
        // trailing padding
        ch = *(fmt++);
        ch = a2i( ch, &fmt, 10, &trailing );
        lz = ' ';
        break;
      }

      switch( ch ) {
      case 0: return;
      case 'c':
        jstrappendc(buf, (char) va_arg( va, int ), buf);
        used_buf++;
        break;
      case 's':
        used_size = jstrappendw( buf, w, lz, va_arg( va, char* ) );
        used_buf += used_size;
        break;
      case 'u':
        ui2a( va_arg( va, unsigned int ), 10, bf );
        used_size = jstrappendw( buf, w, lz, bf );
        used_buf += used_size;
        break;
      case 'd':
        i2a( va_arg( va, int ), bf );
        used_size = jstrappendw( buf, w, lz, bf );
        used_buf += used_size;
        break;
      case 'l':
        l2a( va_arg( va, long int ), bf );
        used_size = jstrappendw( buf, w, lz, bf );
        used_buf += used_size;
        break;
      case 'x':
        ui2a( va_arg( va, unsigned int ), 16, bf );
        used_size = jstrappendw( buf, w, lz, bf );
        used_buf += used_size;
        break;
      case '%':
        jstrappendc(buf, ch, buf);
        used_buf++;
        break;
      }
      trailing -= used_size;
      while (trailing-- > 0) {
        jstrappendc(buf, lz, buf);
        buf++;
        used_buf++;
      }
    }
    KASSERT(used_buf < buf_size, "jformatf provided buffer overflowed: fmt=\"%s\"", fmt);
  }
}

void jformatf( char *buf, int buf_size, char *fmt, ... ) {
  va_list va;
  va_start(va,fmt);
  jformat( buf, buf_size, fmt, va );
  va_end(va);
}
