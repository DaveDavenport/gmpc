#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "debug_printf.h"

#define RED "\x1b[31;01m"
#define DARKRED "\x1b[31;06m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32;06m"
#define YELLOW "\x1b[33;06m"

void debug_printf(int dp,char *format, ...)
{
#ifdef DEBUG
	va_list arglist;
	va_start(arglist,format);
	if(dp == DEBUG_INFO)
	{
		printf(GREEN"INFO:\t"RESET);
	}
	else if(dp == DEBUG_WARNING)
	{
		printf(YELLOW"WARNING:\t"RESET);
	}
	else
	{
		printf(DARKRED"ERROR:\t"RESET);
	}
	vprintf(format, arglist);
	if(format[strlen(format)-1] != '\n')
	{
		printf("\n");
	}
	va_end(arglist);
#endif
}



