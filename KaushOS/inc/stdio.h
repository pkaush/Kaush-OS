/*
*  C Interface:	Stdio.h
*
* Description: Error definitions
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#ifndef __KOS_INC_STDIO_H__
#define __KOS_INC_STDIO_H__

#include	<inc/types.h>
#include	<inc/stdarg.h>

/* Predefined file handles. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/* Standard functions. */
int
printf (const char *String, ...);

int
snprintf(char *String, size_t n, const char *String1, ...);

int
vprintf (const char *, va_list);

int
vsnprintf (char *, size_t, const char *, va_list);
int
putchar (int);

int
puts (const char *);

/* Nonstandard functions. */
void
hex_dump (ULONG_PTR ofs, const void *, size_t size, BOOL ascii);

/* Internal functions. */
void
__vprintf (const char *format, va_list args,
                void (*output) (char, void *), void *aux);
void
__printf (const char *format,
               void (*output) (char, void *), void *aux, ...);


#endif
