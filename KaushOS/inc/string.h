/*
*  C Interface:	String.h
*
* Description: Interface to string operations.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#ifndef KOS_INC_STRING_H
#define KOS_INC_STRING_H

#include <inc/types.h>

int	strlen(const char *s);
int	strnlen(const char *s, size_t size);
char *	strcpy(char *dst, const char *src);
char *	strncpy(char *dst, const char *src, size_t size);
size_t	strlcpy(char *dst, const char *src, size_t size);
int	strcmp(const char *s1, const char *s2);
int	strncmp(const char *s1, const char *s2, size_t size);
char *	strchr(const char *s, char c);
char *	strfind(const char *s, char c);

void *	memset(void *dst, int c, size_t len);
//static __inline void *	memcpy(void *dst, const void *src, size_t len);
void *	memcpy(void *dst, const void *src, size_t len);
void *	memmove(void *dst, const void *src, size_t len);
int	memcmp(const void *s1, const void *s2, size_t len);
void *	memfind(const void *s, int c, size_t len);

long	strtol(const char *s, char **endptr, int base);




static inline int islower (int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper (int c) { return c >= 'A' && c <= 'Z'; }
static inline int isalpha (int c) { return islower (c) || isupper (c); }
static inline int isdigit (int c) { return c >= '0' && c <= '9'; }
static inline int isalnum (int c) { return isalpha (c) || isdigit (c); }
static inline int isxdigit (int c) {
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int isspace (int c) {
  return (c == ' ' || c == '\f' || c == '\n'
          || c == '\r' || c == '\t' || c == '\v');
}
static inline int isblank (int c) { return c == ' ' || c == '\t'; }
static inline int isgraph (int c) { return c > 32 && c < 127; }
static inline int isprint (int c) { return c >= 32 && c < 127; }
static inline int iscntrl (int c) { return (c >= 0 && c < 32) || c == 127; }
static inline int isascii (int c) { return c >= 0 && c < 128; }
static inline int ispunct (int c) {
  return isprint (c) && !isalnum (c) && !isspace (c);
}

static inline int tolower (int c) { return isupper (c) ? c - 'A' + 'a' : c; }
static inline int toupper (int c) { return islower (c) ? c - 'a' + 'A' : c; }

#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))




VOID
RtlCopyUnicodeString(
								IN OUT PUNICODE_STRING DestinationString,
								IN PCUNICODE_STRING SourceString);


LONG
RtlCompareUnicodeString(
								IN PCUNICODE_STRING Str1,
								IN PCUNICODE_STRING Str2,
								IN BOOLEAN  CaseInsensitive);


VOID
RtlInitUnicodeString(
								IN OUT PUNICODE_STRING DestinationString,
					                     IN PCWSTR SourceString);

#endif //End of Header
