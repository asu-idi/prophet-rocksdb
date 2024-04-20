#include<cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>


const int LOGMODE = 0;

void my_printf(const char* format, ...)
{
	printf("Debug => ");
	va_list vp;
	va_start(vp, format);
	vprintf (format, vp);
	va_end  (vp);
	printf  ("\n");
}
