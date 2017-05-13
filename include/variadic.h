#ifndef __VARIADIC_H__
#define __VARIADIC_H__
#ifndef DEBUG_MODE

typedef char *va_list;

#define __va_argsiz(t)  \
  (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)  ((void)0)

#define va_arg(ap, t)  \
  (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#endif
#endif
