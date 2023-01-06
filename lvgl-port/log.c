
#include <stdarg.h>
#include "log.h"

#define BUF_SIZE 512

void log_line(char * what, const char * file, int line, char * fmt, ...)
{
  char buffer[BUF_SIZE];

  va_list ap;
  va_start(ap, fmt); /* Initialize the va_list */

  vsnprintf (buffer, sizeof(buffer), fmt, ap);
  va_end(ap); /* Cleanup the va_list */

  printf("%s: %s(%d): %s\n", what, file, line, buffer);

}