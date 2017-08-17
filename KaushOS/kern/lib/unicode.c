/*
*  C Implementation: Unicode.c
*
* Description: Implemetation of helper functions for handling unicode Strings
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <inc/types.h>
#include <inc/string.h>
#include <inc/assert.h>
#define wcslen strlen



LONG
RtlCompareUnicodeString(
								IN PCUNICODE_STRING Str1,
								IN PCUNICODE_STRING Str2,
								IN BOOLEAN  CaseInsensitive)
{
	unsigned int len;
	LONG ret = 0;


	if (Str1->Length > Str2->Length) {
		return -1;
	}

	if (Str2->Length > Str1->Length) {
		return 1;
	}
	

	ret = strncmp(Str1->Buffer, Str2->Buffer,Str1->Length);

	return ret;
}





VOID
RtlCopyUnicodeString(
	IN OUT PUNICODE_STRING DestinationString,
	IN PCUNICODE_STRING SourceString)
{
	ULONG SourceLength;

	if(SourceString == NULL) {
		DestinationString->Length = 0;

	} else {

		SourceLength = min(DestinationString->MaximumLength, SourceString->Length);
		DestinationString->Length = (USHORT)SourceLength;

		RtlCopyMemory(DestinationString->Buffer,	SourceString->Buffer,SourceLength);

		if (DestinationString->Length < DestinationString->MaximumLength) {
			DestinationString->Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
		}

	}
}




VOID
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
	ULONG DestSize;

	if(SourceString) {

		DestSize = wcslen(SourceString) * sizeof(WCHAR);

		DestinationString->Length = (USHORT)DestSize;
		DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);

	} else {

		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;

	}
	
	DestinationString->Buffer = (PWCHAR)SourceString;
}

