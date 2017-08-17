
#ifndef __KOS_INC_LIST_H__
#define __KOS_INC_LIST_H__

#include <inc/types.h>
#include <inc/assert.h>
#include <inc/interlocked.h>

#define	KLIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type *le_prev;	/* prev element */	\
}

#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
/*	struct type *lh_last;	 End of the list */		\
	ULONG NumberOfElements;				\
}





//Syncronization for the list

#if 0
#if 0 //def	_USE_ERESOURCE_FOR_LIST

#define LIST_LOCK	ERESOURCE
#define LIST_LOCK_INIT(head)		ExInitializeResourceLite(&head->Lock);
#define LIST_ACQUIRE_LOCK_WRITE(head)	ExAcquireResourceExclusiveLite(&head->Lock,NULL)
#define LIST_ACQUIRE_LOCK_READ(head)	ExAcquireResourceSharedLite(&head->Lock, NULL)
#define LIST_RELEASE_LOCK(head)	ExReleaseResourceLite(&head->Lock)

#else

#define LIST_LOCK	ULONG
#define LIST_LOCK_INIT(head)		(head->Lock = 0)

#define LIST_ACQUIRE_LOCK_WRITE(head)			\
						while(InterlockedExchange(&((head)->Lock), 1) != 0) {	\
							/*sched();*/		\
						}			
#define LIST_ACQUIRE_LOCK_READ(head)			\
						while(InterlockedExchange(&((head)->Lock), 1) != 0) {	\
							/*sched();*/		\
						}			
#define LIST_RELEASE_LOCK(head)	InterlockedExchange(&((head)->Lock), 0)

#endif
#endif

#define	LIST_NUM_ELEMENTS(head) ((head)->NumberOfElements)



#define	LIST_INIT(head) do {						\
	LIST_FIRST((head)) = NULL;					\
/*	LIST_LOCK_INIT((head));*/						\
	LIST_NUM_ELEMENTS(head) = 0;			\
} while (0)



#define	LIST_INIT_ENTRY(elm,link)		do {	\
		elm->link.le_next = NULL;		\
		elm->link.le_prev = NULL;		\
	} while(0)



#define	LIST_FIRST(head)	((head)->lh_first)

#define	LIST_EMPTY(head)	((LIST_FIRST(head)) == NULL)

#define	LIST_LAST(head,link)	((LIST_EMPTY(head))?NULL:((LIST_FIRST(head))->link.le_prev))


// These are not safe functions -- only for internal use
// LIST_NEXT and LIST_PREV for Internal use
#define	LIST_NEXT_INTERNAL(elm, link)	((elm)->link.le_next)			
#define	LIST_PREV_INTERNAL(elm, link)	 ((elm)->link.le_prev)



// do check for NULL in the return values
#define	LIST_NEXT(head, elm, link)		\
		((LIST_NEXT_INTERNAL(elm, link) == LIST_FIRST(head))? NULL: ((elm)->link.le_next))
			

#define	LIST_PREV(head, elm, link)				\
		((LIST_PREV_INTERNAL(elm, link) == LIST_LAST(head,link))? NULL: ((elm)->link.le_prev))


#define	LIST_FOREACH(var, head, link)					\
	for ((var) = LIST_FIRST((head));				\
	    (var);							\
	    (var) = LIST_NEXT(head, (var), link))


//insert elm after the listelm in the list

#define	LIST_INSERT_AFTER(head, listelm, elm, link) do {			\
	LIST_NEXT_INTERNAL(elm,link) = LIST_NEXT_INTERNAL(listelm,link);	\
	LIST_NEXT_INTERNAL(listelm,link) = (elm);						\
	LIST_PREV_INTERNAL(elm,link) = (listelm);						\
	LIST_PREV_INTERNAL(LIST_NEXT_INTERNAL(elm,link), link) = (elm);		\
	LIST_NUM_ELEMENTS(head) ++;										\
} while (0)



#define	LIST_INSERT_BEFORE(head, listelm, elm, link)			\
	LIST_INSERT_AFTER(head, LIST_PREV_INTERNAL(listelm, link), elm, link)

#define	LIST_INSERT_HEAD(head, elm, link) do {				\
	if (LIST_EMPTY(head)) {										\
		LIST_FIRST(head) = (elm);								\
		LIST_PREV_INTERNAL(elm,link) = elm;					\
		LIST_NEXT_INTERNAL(elm,link) = elm;					\
		ASSERT(LIST_NUM_ELEMENTS(head) == 0);					\
		LIST_NUM_ELEMENTS(head) ++;						\
	} else {															\
		LIST_INSERT_BEFORE(head, LIST_FIRST(head),elm,link);		\
		LIST_FIRST(head) = (elm);				\
	}											\
} while (0)




#define	LIST_INSERT_TAIL(head, elm, link) do {				\
	if (LIST_EMPTY(head)) {										\
		LIST_INSERT_HEAD(head, elm, link);						\
	} else {															\
		LIST_INSERT_BEFORE(head, LIST_FIRST(head),elm,link);		\
	}									\
} while (0)



#define	LIST_REMOVE(head, elm, link) do {					\
	if ((elm) == LIST_FIRST(head)) {							\
/*if elm is the only element in the list LIST_NEXT will retun NULL*/	\
		LIST_FIRST(head) = LIST_NEXT(head,elm,link);		\
	}															\
	/* we do not need this if elm is the only element,	*/		\
		/* but doesn't harm us in any way */					\
	LIST_NEXT_INTERNAL(LIST_PREV_INTERNAL(elm,link),link)	\
				= LIST_NEXT_INTERNAL(elm,link);							\
	LIST_PREV_INTERNAL( LIST_NEXT_INTERNAL(elm,link),link)		\
				= LIST_PREV_INTERNAL(elm,link);					\
	LIST_NUM_ELEMENTS(head) --;								\
} while (0)



#define	LIST_REMOVE_HEAD(head, elm, link) do {					\
				if(LIST_EMPTY(head)) {								\
					elm = NULL;											\
				} else {													\
					elm = LIST_FIRST(head);								\
					LIST_REMOVE(head,elm,link);							\
				}														\
			} while(0)

#define	LIST_REMOVE_TAIL(head, elm, link) do {					\
				if(LIST_EMPTY(head)) {								\
					elm = NULL; 										\
				} else {													\
					elm = LIST_LAST(head,link); 							\
					LIST_REMOVE(head,elm,link); 						\
				}														\
			} while(0)
				



#if 0
static void
List_testing()
{

		struct Frob
		{
			int frobozz;
			KLIST_ENTRY(Frob) link; /* this contains the list element pointers */
		};
	
		struct Frob a[20];
		struct Frob *ftest1;
		LIST_HEAD(Frob_list, Frob); 	/* defines struct Frob_list as a list of Frob */
		struct Frob_list flist; 		/* declare a Frob list */
	
		LIST_INIT(&flist);			/* clear flist (globals are cleared anyway) */
	
		ASSERT(LIST_EMPTY(&flist)); 		/* check whether list is empty */
		ASSERT(LIST_FIRST(&flist) == NULL);
		ASSERT(LIST_LAST(&flist,link) == NULL);
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 0);
		
		LIST_INSERT_TAIL(&flist, &a[0], link);
		// head -> a[0]	
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 1);
		ASSERT(LIST_FIRST(&flist) == &a[0]);
		ASSERT(LIST_LAST(&flist,link) == &a[0]);
		ASSERT(LIST_NEXT_INTERNAL(&a[0],link) == &a[0]);
		ASSERT(LIST_PREV_INTERNAL(&a[0],link) == &a[0]);

		ASSERT(LIST_NEXT(&flist, &a[0],link) == NULL);
		ASSERT(LIST_PREV(&flist, &a[0],link) == NULL);

		LIST_INSERT_TAIL(&flist, &a[1], link);
		//head -> a[0] => a[1]
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 2);
		ASSERT(LIST_FIRST(&flist) == &a[0]);
		ASSERT(LIST_LAST(&flist,link) == &a[1]);
		ASSERT(LIST_NEXT_INTERNAL(&a[1],link) == &a[0]);
		ASSERT(LIST_PREV_INTERNAL(&a[1],link) == &a[0]);

		ASSERT(LIST_NEXT(&flist, &a[1],link) == NULL);
		ASSERT(LIST_PREV(&flist, &a[0],link) == NULL);

		LIST_INSERT_HEAD(&flist, &a[2],link);
		// head -> a[2] => a[0] => a[1]
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 3);
		ASSERT(LIST_NEXT(&flist, &a[2],link) == &a[0]);
		ASSERT(LIST_NEXT_INTERNAL(&a[2],link) == &a[0]);
		ASSERT(LIST_PREV(&flist, &a[2],link) == NULL);
	
		ASSERT(LIST_NEXT(&flist, &a[0],link) == &a[1]);
		ASSERT(LIST_PREV(&flist, &a[0],link) == &a[2]);

		ASSERT(LIST_NEXT(&flist, &a[1],link) == NULL);
		ASSERT(LIST_PREV_INTERNAL(&a[1],link) == &a[0]);
//		ASSERT(LIST_PREV(&flist, &a[1],link) == &a);


#if 0
int i;
	for(i = 0; i<10; i++){
		LIST_INSERT_HEAD(&flist, &a[i], frob_link); /* add g as first element in list */
	}

	
		printf("head %x link %x\n\n\n", &flist,  flist.lh_first);
		
	for(f=LIST_FIRST(&flist); f != 0;	/* iterate over elements in flist */
		f = LIST_NEXT(&flist, f, frob_link)){
		printf(" node %x  link %x prev -> %x next -> %x\n",f, &f->frob_link, f->frob_link.le_prev, f->frob_link.le_next);
	}
	
	printf("\n\n\n");
#endif	



		LIST_REMOVE_HEAD( &flist, ftest1,link);
		// head -> a[0] => a[1]
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 2);
		ASSERT(ftest1 == &a[2]);


		ASSERT(LIST_NEXT_INTERNAL(&a[1],link) == &a[0]);
		ASSERT(LIST_PREV_INTERNAL(&a[1],link) == &a[0]);

		ASSERT(LIST_NEXT(&flist, &a[1],link) == NULL);
		ASSERT(LIST_PREV(&flist, &a[0],link) == NULL);



		LIST_REMOVE_TAIL( &flist, ftest1,link);
		// head -> a[0]
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 1);
		ASSERT(ftest1 == &a[1]);

		ASSERT(LIST_NEXT_INTERNAL(&a[0],link) == &a[0]);
		ASSERT(LIST_PREV_INTERNAL(&a[0],link) == &a[0]);

		ASSERT(LIST_NEXT(&flist, &a[0],link) == NULL);
		ASSERT(LIST_PREV(&flist, &a[0],link) == NULL);
	
		LIST_REMOVE_TAIL( &flist, ftest1,link);
		ASSERT(LIST_NUM_ELEMENTS(&flist) == 0);
		ASSERT(LIST_EMPTY(&flist));

//		kprintf("List Testing has passed\n");
}
#endif

#if 0
int
main()
{

List_testing();

}
#endif




/************************************
	Singly Linked List
************************************/
//#if 0


typedef struct _SINGLE_LIST_ENTRY {
		struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY,*PSINGLE_LIST_ENTRY;


#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define _SLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY
//typedef union _SLIST_HEADER {
//	ULONGLONG Alignment;
//	struct {

typedef struct _SLIST_HEADER {

		SLIST_ENTRY Next;
		USHORT Depth;
		USHORT Sequence;
//	}u1;
} SLIST_HEADER, *PSLIST_HEADER;



#define InitializeSListHead(_SListHead) \
			(_SListHead)->Next = 0
//			(_SListHead)->Alignment = 0


static INLINE
PSLIST_ENTRY
InterlockedPushEntrySList(
			IN PSLIST_HEADER ListHead,
			IN PSLIST_ENTRY ListEntry
			)
{
    
	PSINGLE_LIST_ENTRY FirstEntry, NextEntry;
	PSINGLE_LIST_ENTRY Entry = (PVOID)ListEntry, 
							Head = (PVOID)ListHead;
    
	FirstEntry = Head->Next;
	do {
		Entry->Next = FirstEntry;
		NextEntry = FirstEntry;
		FirstEntry = (PVOID)InterlockedCompareExchangePointers(
								(PLONG)Head,
								(LONG)Entry,
								(LONG)FirstEntry);
	} while (FirstEntry != NextEntry);
    
	return FirstEntry;
}


static INLINE
PSLIST_ENTRY
InterlockedPopEntrySList(IN PSLIST_HEADER ListHead)
{
	PSINGLE_LIST_ENTRY FirstEntry, NextEntry, Head = (PVOID)ListHead;
    
	FirstEntry = Head->Next;
	do {
		if (!FirstEntry) {
			return NULL;
		}

		NextEntry = FirstEntry;
		FirstEntry = (PVOID) InterlockedCompareExchangePointers(
								(PLONG)Head,
								(LONG)FirstEntry->Next,
								(LONG)FirstEntry);
		
	} while (FirstEntry != NextEntry);

	return (FirstEntry);    
}


static INLINE
PSINGLE_LIST_ENTRY
ExInterlockedFlushSList(IN PSLIST_HEADER ListHead)
{
    return (PVOID)InterlockedExchange((PLONG)&ListHead->Next.Next, (LONG)NULL);
}

static INLINE
USHORT
QueryDepthSList(IN PSLIST_HEADER ListHead)
{
	return(ListHead->Depth);
}



/**********************************
	Second form of doubly linked list
**********************************/


typedef struct _LIST_ENTRY
{
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY,*PLIST_ENTRY;


static INLINE
VOID
InitializeListHead(
                   IN PLIST_ENTRY ListHead
                   )
{
	ListHead->Flink = ListHead->Blink = ListHead;
}

static INLINE
VOID
InsertHeadList(
               IN PLIST_ENTRY ListHead,
               IN PLIST_ENTRY Entry
               )
{
	PLIST_ENTRY OldFlink;
	OldFlink = ListHead->Flink;
	Entry->Flink = OldFlink;
	Entry->Blink = ListHead;
	OldFlink->Blink = Entry;
	ListHead->Flink = Entry;
}

static INLINE
VOID
InsertTailList(
               IN PLIST_ENTRY ListHead,
               IN PLIST_ENTRY Entry
               )
{
	PLIST_ENTRY OldBlink;
	OldBlink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = OldBlink;
	OldBlink->Flink = Entry;
	ListHead->Blink = Entry;
}

static INLINE
BOOLEAN
IsListEmpty(
            IN const LIST_ENTRY * ListHead
            )
{
	return (BOOLEAN)(ListHead->Flink == ListHead);
}

static INLINE
BOOLEAN
RemoveEntryList(
                IN PLIST_ENTRY Entry)
{
	PLIST_ENTRY OldFlink;
	PLIST_ENTRY OldBlink;

	OldFlink = Entry->Flink;
	OldBlink = Entry->Blink;
	OldFlink->Blink = OldBlink;
	OldBlink->Flink = OldFlink;
	return (BOOLEAN)(OldFlink == OldBlink);
}

static INLINE
PLIST_ENTRY
RemoveHeadList(
               IN PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Flink;
	PLIST_ENTRY Entry;

	Entry = ListHead->Flink;
	Flink = Entry->Flink;
	ListHead->Flink = Flink;
	Flink->Blink = ListHead;
	return Entry;
}

static INLINE
PLIST_ENTRY
RemoveTailList(
               IN PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Blink;
	PLIST_ENTRY Entry;

	Entry = ListHead->Blink;
	Blink = Entry->Blink;
	ListHead->Blink = Blink;
	Blink->Flink = ListHead;
	return Entry;
}


/*****************************

*****************************/




#endif
