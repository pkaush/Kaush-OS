/*
*  C Implementation: File.c
*
* Description: File and Device driver related implementations. Remember KOS treates Devices also as files.
*
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/


#include <io.h>



KSTATUS
IoCreateFileObjectType()
{


	OBJECT_TYPE_INFO Object_Type_Info;
	memset(&Object_Type_Info, 0 , sizeof(OBJECT_TYPE_INFO));
	
	Object_Type_Info.ObjectType = OBJECT_TYPE_FILE;
	Object_Type_Info.PoolType = PagedPool; // Initial value
	LIST_INIT(&Object_Type_Info.Object_List_Head);
	Object_Type_Info.ObjectMethods
	
	return (ObCreateObjectType(&Object_Type_Info));

}


