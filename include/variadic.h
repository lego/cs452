#if !defined(__VARIADIC__H) && !DEBUG_MODE
#define __VARIADIC__H

/*
 * NOTE: this was yoink'd and resembles GNU's stdarg.h but it's a bit different
 * so we're using it in ARM world
 */

typedef char *va_list;

#define __va_argsiz(t)  \
  (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)  ((void)0)

#define va_arg(ap, t)  \
  (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#elif DEBUG_MODE
// Part of the C std library for new compilers
#include <stdarg.h>
#endif
