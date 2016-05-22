#include "verbosity.h"

void logger_init(void)
{

}

void logger_shutdown(void)
{

}

void logger_send(const char *__format, ...)
{
   va_list args;

   va_start(args, __format);
   logger_send_v(__format, args);
   va_end(args);
}

void logger_send_v(const char *__format, va_list args)
{
   
}