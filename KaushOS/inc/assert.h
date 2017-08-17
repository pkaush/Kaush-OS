/*
*  C Interface: assert.h
*
* Description: Interface to ASSERT, warn and KeBugCheck for both user mode and kernel mode.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#ifndef KOS_INC_ASSERT_H
#define KOS_INC_ASSERT_H


#include <inc/stdio.h>


void lprint(char *str);

void _warn(const char*, int, const char*, ...);
void _KeBugCheck(const char*, int, const char*, ...) __attribute__((noreturn));


#define DEBUG

#ifdef DEBUG
#define DBGPRINT(fmt, args...) 		kprintf(fmt, ## args)
#else
#define DBGPRINT(fmt, args...)
#endif


#ifdef TRACE
#define TRACEPRINT(fmt, args...)		kprintf(fmt, ## args)
#else
#define TRACEPRINT(fmt, args...)
#endif


#define LOG_CRIT		0
#define LOG_ERROR	1
#define LOG_DEBUG	2
#define LOG_INFO	3
#define LOG_CRAP		4
#define LOG_MAX		5




#define Log(level,fmt, args...)				\
do{											\
	if ((level) <= LOG_INFO) {				\
		 		kprintf(fmt, ## args);		\
	}										\
}while(0)




#define TRACETHIS	kprintf("%s:%d\n",__FUNCTION__, __LINE__)


#define KeBugCheck(...) _KeBugCheck(__FILE__, __LINE__, __VA_ARGS__)

#ifdef DEBUG
#define DBGKeBugCheck(...) _KeBugCheck(__FILE__, __LINE__, __VA_ARGS__)
#else
#define DBGKeBugCheck(...)

#endif

#ifdef DEBUG	// ASSERTs should only trigger in DEBUG mode
#define ASSERT(x)		\
	do { if (!(x)) KeBugCheck("assertion failed: %s", #x); } while (0)


#define ASSERT1(x, y)		\
		do { if (!(x)) KeBugCheck("assertion failed: %s, 0x%x", #x, y); } while (0)


#define ASSERT2(x,y,z)		\
			do { if (!(x)) KeBugCheck("assertion failed: %s, 0x%x 0x%x", #x, y, z); } while (0)


#else
#define ASSERT(x)	
#define ASSERT1(x, y)
#define ASSERT2(x,y,z)
#endif

// STATIC_ASSERT(x) will generate a compile-time error if 'x' is false.
#define STATIC_ASSERT(x)	switch (x) case 0: case (x):

#endif //End of Header
