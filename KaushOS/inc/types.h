/*
*  C Interface:	Types.h
*
* Description: Typedefs of various data types
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/



#ifndef KOS_INC_TYPES_H
#define KOS_INC_TYPES_H


#if 0
#ifndef TRACEEXEC
#define TRACEEXEC	kprintf("TRACE[%s:%d]\n", __FUNCTION__, __LINE__)
#endif

#else
#ifndef TRACEEXEC
#define TRACEEXEC 
#endif

#endif


#ifndef NULL
#define NULL ((void*) 0)
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif


#ifndef INLINE
#define INLINE inline
#endif

#ifndef FORCEINLINE
//#define FORCEINLINE extern __inline__ __attribute__((__always_inline__,__gnu_inline__))
#define FORCEINLINE static __inline__ __attribute__((always_inline))
#endif



#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)




//Max Length for name
#define MAX_NAME_LENGTH		64







// Represents true-or-false values
#if 0
typedef int BOOLEAN;
typedef bool BOOL;
typedef bool BOOLEAN;

#define	TRUE	1
#define	FALSE	0
#endif


typedef enum {
	FALSE,
	TRUE
} BOOLEAN, BOOL;


// Explicitly-sized versions of integer types


typedef __signed char int8_t;
typedef __signed char CHAR;
typedef __signed char *PCHAR;
#define INT8_MAX 127
#define INT8_MIN (-INT8_MAX - 1)

typedef __signed short int int16_t;
typedef __signed short int SHORT;
typedef __signed short int *PSHORT;
#define INT16_MAX 32767
#define INT16_MIN (-INT16_MAX - 1)

typedef __signed int int32_t;
typedef __signed int INT;
typedef __signed int LONG;
typedef __signed int *PLONG;
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX - 1)

typedef __signed long long int int64_t;
typedef __signed long long int LONG64;
typedef __signed long long int *PLONG64;
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX - 1)

typedef unsigned char uint8_t;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef unsigned char *PBYTE;
typedef unsigned char *PUCHAR;
#define UINT8_MAX 255

typedef unsigned short int uint16_t;
typedef unsigned short int USHORT;
typedef unsigned short	int WORD;
typedef unsigned char WCHAR; // No support for wide char
typedef WCHAR *PWCHAR; // No support for wide char

typedef unsigned short int *PUSHORT;
#define UINT16_MAX 65535

typedef unsigned int uint32_t;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef unsigned long *PULONG;
#define UINT32_MAX 4294967295U

typedef unsigned long long int uint64_t;
typedef unsigned long long int ULONG64;
typedef unsigned long long int *PULONG64;
#define UINT64_MAX 18446744073709551615ULL


// for 32 bit arch
typedef int32_t intptr_t;
typedef int32_t LONG_PTR;
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX

typedef uint32_t uintptr_t;
typedef uint32_t ULONG_PTR;
#define UINTPTR_MAX UINT32_MAX

typedef int64_t intmax_t;
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX

typedef uint64_t uintmax_t;
#define UINTMAX_MAX UINT64_MAX

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX

#define SIZE_MAX UINT32_MAX


typedef void VOID;
typedef void *PVOID;


typedef PCHAR PSTR;
typedef const PCHAR PCSTR;



// We are not supporting Wide Char right now. Later!!
typedef PWCHAR PWSTR;
typedef const PWCHAR PCWSTR;

typedef struct _STRING {
		USHORT Length;
		USHORT MaximumLength;
		PSTR  Buffer;
} STRING, *PSTRING;


typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PSTR  Buffer; // We do not yet support UNICODE, next release!!
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

#define ANSI_NULL ((CHAR)0)
#define UNICODE_NULL ((WCHAR)0)



typedef ULONG64	LARGE_INTEGER;
typedef ULONG64 *PLARGE_INTEGER;




typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;



//typedef uint32_t size_t;
// ssize_t is a signed version of ssize_t, used in case there might be an
// error return.
typedef int32_t ssize_t;
typedef int32_t off_t;



typedef ULONG_PTR  PHYSICAL_ADDRESS;
// Virtual will be "PVOID"


typedef ULONG	KAFFINITY;



// Efficient min and max operations
#define MIN(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a <= __b ? __a : __b;					\
})
#define MAX(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a >= __b ? __a : __b;					\
})

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)						\
({								\
	ULONG __a = (ULONG) (a);				\
	(typeof(a)) (__a - __a % (n));				\
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)						\
({								\
	ULONG __n = (ULONG) (n);				\
	(typeof(a)) (ROUNDDOWN((ULONG) (a) + __n - 1, __n));	\
})

// Return the offset of 'member' relative to the beginning of a struct type
#define offsetof(type, member)  ((size_t) (&((type*)0)->member))


// Pulled out from DDK
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (CHAR*)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))



#endif //End of Header
