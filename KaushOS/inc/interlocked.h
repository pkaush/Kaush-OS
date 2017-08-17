/*
*  C Interface: Interlocked.h
*
* Description: Interlocked operations
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#ifndef __KOS_INC_INTERLOCKED_H__
#define __KOS_INC_INTERLOCKED_H__




FORCEINLINE
LONG 
InterlockedExchangeAdd(
						volatile PLONG const Addend,
						const LONG Value
						)
{

	return __sync_fetch_and_add(Addend, Value);
}



FORCEINLINE
LONG 
InterlockedIncrement(
						IN PLONG  Addend
						)
{
	return(InterlockedExchangeAdd(Addend, 1)+1);
}


FORCEINLINE
LONG 
InterlockedDecrement(
						IN PLONG  Addend
						)
{
	return (InterlockedExchangeAdd(Addend, -1)-1);
}


FORCEINLINE
CHAR
InterlockedCompareExchange8(
							volatile PCHAR const Destination,
							const CHAR Exchange,
							const CHAR Comperand
							)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE
SHORT
InterlockedCompareExchange16(
							volatile SHORT * const Destination,
							const SHORT Exchange,
							const SHORT Comperand
							)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE
LONG
InterlockedCompareExchange(
							IN OUT volatile PLONG const Destination,
							IN const LONG Exchange,
							IN const LONG Comperand
							)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE
PVOID
InterlockedCompareExchangePointer(
							IN OUT PVOID volatile * const Destination,
							IN PVOID const Exchange,
							IN PVOID const Comperand
							)
/*++

Parameters

	Destination [in, out]

	    A pointer to a pointer to the destination value.
	Exchange [in]

	    The exchange value.
	Comparand [in]

	    The value to compare to Destination.

Return Value

	The function returns the initial value of the Destination parameter.

Remarks

	The function compares the Destination value with the Comparand value.
	If the Destination value is equal to the Comparand value, the Exchange
	value is stored in the address specified by Destination. Otherwise, no operation is performed.
--*/

{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE
LONG
InterlockedExchange(
							volatile PLONG const Target,
							const LONG Value
							)
{
	
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}


FORCEINLINE
PVOID
InterlockedExchangePointer(
							PVOID volatile * const Target,
							PVOID const Value
							)
{

	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

FORCEINLINE 
LONG
InterlockedExchangeAdd16(
							volatile SHORT * const Addend,
							const SHORT Value
							)
{
	return __sync_fetch_and_add(Addend, Value);
}



FORCEINLINE 
CHAR 
InterlockedAnd8(
						volatile PCHAR const value,
						const CHAR mask
						)
{
	return __sync_fetch_and_and(value, mask);
}

FORCEINLINE
SHORT 
InterlockedAnd16(
						volatile SHORT * const value,
						const SHORT mask
						)
{
	return __sync_fetch_and_and(value, mask);
}

FORCEINLINE
LONG
InterlockedAnd(
						volatile PLONG const value,
						const LONG mask)
{
	return __sync_fetch_and_and(value, mask);
}


FORCEINLINE 
CHAR
InterlockedOr8(volatile PCHAR const value, const CHAR mask)
{
	return __sync_fetch_and_or(value, mask);
}

FORCEINLINE
SHORT 
InterlockedOr16(volatile SHORT * const value, const SHORT mask)
{
	return __sync_fetch_and_or(value, mask);
}

FORCEINLINE 
LONG
InterlockedOr(volatile PLONG const value, const LONG mask)
{
	return __sync_fetch_and_or(value, mask);
}


FORCEINLINE
CHAR
InterlockedXor8(volatile PCHAR const value, const CHAR mask)
{
	return __sync_fetch_and_xor(value, mask);
}

FORCEINLINE
SHORT
InterlockedXor16(volatile SHORT * const value, const SHORT mask)
{
	return __sync_fetch_and_xor(value, mask);
}

FORCEINLINE
LONG
InterlockedXor(volatile PLONG const value, const LONG mask)
{
	return __sync_fetch_and_xor(value, mask);
}



#endif
