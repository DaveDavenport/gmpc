#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

enum {
	DEBUG_INFO,
	DEBUG_WARNING,
	DEBUG_ERROR
};

void debug_printf(int dp, char *format, ...);



#endif
