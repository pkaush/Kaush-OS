/*
*  C Implementation: Ide.c
*
* Description: Basic implementation of IDE driver
*	This is the temporary implementation for IDE driver. The code is derived from Pintos.
*
* Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
*
* Copyright: See COPYRIGHT file that comes with this distribution
*
*/

#include "ide.h"
#include <kern/ke/ke.h>

#define DISK_SECTOR_SIZE 512
#define SECTOR_SIZE	512
#define BLOCK_SIZE	(SECTOR_SIZE*4)


#define _NEW_IDE_CODE_ 1
#define _TEST_IDE_CODE_ 1

#if 1



typedef uint32_t disk_sector_t;

KDPC IdeDpcObject;


VOID
IdeDpcRoutine (
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	);


#define semaphore _KSEMAPHORE
//#define lock _KMUTANT
//typedef _KMUTANT lock

#define lock_init(x) KeInitializeMutant(x, FALSE)
#define sema_init(x,y) KeInitializeSemaphore(x, y, 100)
#define intr_frame _KTRAPFRAME

#define false FALSE
#define true TRUE


//void intr_register_ext (uint8_t vec_no, intr_handler_func *handler,const char *name) 
//
/*VOID
KiCreateInterruptObject(
					IN ULONG Vector,
					IN ULONG DPL,
					IN PINTERRUPT_HANDLER Routine,
					IN const PCHAR InterruptName
					)
*/

#define intr_register_ext(a,b,c) KiCreateInterruptObject(a, 0, b,c)


#define NOT_REACHED() KeBugCheck("Should not reach here\n")


#define lock_acquire(lock) KeWaitForSingleObject(lock,Executive, KernelMode, FALSE,NULL)

#define PANIC KeBugCheck


#define lock_release(lock) KeReleaseMutant(lock, 0, FALSE, FALSE)

#define sema_down(completion_wait) KeWaitForSingleObject(completion_wait,Executive, KernelMode, FALSE,	NULL)

#define INTR_ON TRUE

#define intr_get_level() HalAreInterruptsEnabled()

#define sema_up(completion_wait) KeReleaseSemaphore(completion_wait,0, 1,FALSE)


#define barrier() asm volatile ("" : : : "memory")

#define NO_INLINE __attribute__ ((noinline))


static ULONG64   loops_per_tick;

typedef BOOL bool;

static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}


/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * 1193/ (denom / 1000)); 
}



#define timer_usleep(us)  real_time_sleep (us, 1000 * 1000)
#define timer_msleep(ms)   real_time_sleep (ms, 1000)
#define timer_nsleep(ns) real_time_sleep (ns, 1000 * 1000 * 1000)






/* ATA command block port addresses. */
#define reg_data(CHANNEL) ((CHANNEL)->reg_base + 0)     /* Data. */
#define reg_error(CHANNEL) ((CHANNEL)->reg_base + 1)    /* Error. */
#define reg_nsect(CHANNEL) ((CHANNEL)->reg_base + 2)    /* Sector Count. */
#define reg_lbal(CHANNEL) ((CHANNEL)->reg_base + 3)     /* LBA 0:7. */
#define reg_lbam(CHANNEL) ((CHANNEL)->reg_base + 4)     /* LBA 15:8. */
#define reg_lbah(CHANNEL) ((CHANNEL)->reg_base + 5)     /* LBA 23:16. */
#define reg_device(CHANNEL) ((CHANNEL)->reg_base + 6)   /* Device/LBA 27:24. */
#define reg_status(CHANNEL) ((CHANNEL)->reg_base + 7)   /* Status (r/o). */
#define reg_command(CHANNEL) reg_status (CHANNEL)       /* Command (w/o). */

/* ATA control block port addresses.
   (If we supported non-legacy ATA controllers this would not be
   flexible enough, but it's fine for what we do.) */
#define reg_ctl(CHANNEL) ((CHANNEL)->reg_base + 0x206)  /* Control (w/o). */
#define reg_alt_status(CHANNEL) reg_ctl (CHANNEL)       /* Alt Status (r/o). */

/* Alternate Status Register bits. */
#define STA_BSY 0x80            /* Busy. */
#define STA_DRDY 0x40           /* Device Ready. */
#define STA_DRQ 0x08            /* Data Request. */

/* Control Register bits. */
#define CTL_SRST 0x04           /* Software Reset. */

/* Device Register bits. */
#define DEV_MBS 0xa0            /* Must be set. */
#define DEV_LBA 0x40            /* Linear based addressing. */
#define DEV_DEV 0x10            /* Select device: 0=master, 1=slave. */

/* Commands.
   Many more are defined but this is the small subset that we
   use. */
#define CMD_IDENTIFY_DEVICE 0xec        /* IDENTIFY DEVICE. */
#define CMD_READ_SECTOR_RETRY 0x20      /* READ SECTOR with retries. */
#define CMD_WRITE_SECTOR_RETRY 0x30     /* WRITE SECTOR with retries. */

/* An ATA device. */
struct disk 
  {
    char name[8];               /* Name, e.g. "hd0:1". */
    struct channel *channel;    /* Channel disk is on. */
    int dev_no;                 /* Device 0 or 1 for master or slave. */

    bool is_ata;                /* 1=This device is an ATA disk. */
    disk_sector_t capacity;     /* Capacity in sectors (if is_ata). */

    long long read_cnt;         /* Number of sectors read. */
    long long write_cnt;        /* Number of sectors written. */
  };

/* An ATA channel (aka controller).
   Each channel can control up to two disks. */
struct channel 
  {
    char name[8];               /* Name, e.g. "hd0". */
    uint16_t reg_base;          /* Base I/O port. */
    uint8_t irq;                /* Interrupt in use. */

    struct _KMUTANT lock;           /* Must acquire to access the controller. */
    bool expecting_interrupt;   /* True if an interrupt is expected, false if
                                   any interrupt would be spurious. */
    struct semaphore completion_wait;   /* Up'd by interrupt handler. */

    struct disk devices[2];     /* The devices on this channel. */
  };

/* We support the two "legacy" ATA channels found in a standard PC. */
#define CHANNEL_CNT 2
static struct channel channels[CHANNEL_CNT];

static void reset_channel (struct channel *);
static bool check_device_type (struct disk *);
static void identify_ata_device (struct disk *);

static void select_sector (struct disk *, disk_sector_t);
static void issue_pio_command (struct channel *, uint8_t command);
static void input_sector (struct channel *, void *);
static void output_sector (struct channel *, const void *);

static void wait_until_idle (const struct disk *);
static bool wait_while_busy (const struct disk *);
static void select_device (const struct disk *);
static void select_device_wait (const struct disk *);

static void interrupt_handler (struct intr_frame *);



#define PRDSNu "u"

void disk_init (void);
void disk_print_stats (void);

struct disk *disk_get (int chan_no, int dev_no);
disk_sector_t disk_size (struct disk *);
void disk_read_internal (struct disk *, disk_sector_t, void *);
void disk_write_internal (struct disk *, disk_sector_t, const void *);


VOID
IdeDpcRoutine (
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)
{

  struct channel *c;

  for (c = channels; c < channels + CHANNEL_CNT; c++)
//    if (TrapFrame->TrapFrame_trapno == c->irq)
      {
        if (c->expecting_interrupt) 
          {
//            inb (reg_status (c));               /* Acknowledge interrupt. */
//            sema_up (&c->completion_wait);      /* Wake up waiter. */
		KeReleaseSemaphore(&c->completion_wait,
								0, 1,FALSE);
          }
//        else
//          printf ("%s: unexpected interrupt\n", c->name);
//        return;
      }



}







/* Format specifier for printf(), e.g.:
   printf ("sector=%"PRDSNu"\n", sector); */



/* Initialize the disk subsystem and detect disks. */
void
disk_init (void) 
{
  size_t chan_no;

  for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++)
    {
      struct channel *c = &channels[chan_no];
      int dev_no;

loops_per_tick = 1u << 10;

      /* Initialize channel. */
      snprintf (c->name, sizeof c->name, "hd%zu", chan_no);
      switch (chan_no) 
        {
        case 0:
          c->reg_base = 0x1f0;
          c->irq = 14 + 0x20;
          break;
        case 1:
          c->reg_base = 0x170;
          c->irq = 15 + 0x20;
          break;
        default:
          NOT_REACHED ();
        }
	  
      lock_init (&c->lock);
      c->expecting_interrupt = false;
      sema_init (&c->completion_wait, 0);
 
      /* Initialize devices. */
      for (dev_no = 0; dev_no < 2; dev_no++)
        {
          struct disk *d = &c->devices[dev_no];
          snprintf (d->name, sizeof d->name, "%s:%d", c->name, dev_no);
          d->channel = c;
          d->dev_no = dev_no;

          d->is_ata = false;
          d->capacity = 0;

          d->read_cnt = d->write_cnt = 0;
        }

      /* Register interrupt handler. */
      intr_register_ext (c->irq, interrupt_handler, c->name);


	  
	  KeInitializeDpc(&IdeDpcObject,
		  &IdeDpcRoutine,NULL);


      /* Reset hardware. */
      reset_channel (c);

      /* Distinguish ATA hard disks from other devices. */
      if (check_device_type (&c->devices[0]))
        check_device_type (&c->devices[1]);

      /* Read hard disk identity information. */
      for (dev_no = 0; dev_no < 2; dev_no++)
        if (c->devices[dev_no].is_ata)
          identify_ata_device (&c->devices[dev_no]);
    }
}


/* Returns the disk numbered DEV_NO--either 0 or 1 for master or
   slave, respectively--within the channel numbered CHAN_NO.

   Pintos uses disks this way:
        0:0 - boot loader, command line args, and operating system kernel
        0:1 - file system
        1:0 - scratch
        1:1 - swap
*/
struct disk *
disk_get (int chan_no, int dev_no) 
{
  ASSERT (dev_no == 0 || dev_no == 1);

  if (chan_no < (int) CHANNEL_CNT) 
    {
      struct disk *d = &channels[chan_no].devices[dev_no];
      if (d->is_ata)
        return d; 
    }
  return NULL;
}



/* Prints disk statistics. */
void
disk_print_stats (void) 
{
  int chan_no;

  for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++) 
    {
      int dev_no;

      for (dev_no = 0; dev_no < 2; dev_no++) 
        {
          struct disk *d;

		  d = disk_get (chan_no, dev_no);
          if (d != NULL && d->is_ata) 
            printf ("%s: %lld reads, %lld writes\n",
                    d->name, d->read_cnt, d->write_cnt);
        }
    }
}


/* Returns the size of disk D, measured in DISK_SECTOR_SIZE-byte
   sectors. */
disk_sector_t
disk_size (struct disk *d) 
{
  ASSERT (d != NULL);
  
  return d->capacity;
}

/* Reads sector SEC_NO from disk D into BUFFER, which must have
   room for DISK_SECTOR_SIZE bytes.
   Internally synchronizes accesses to disks, so external
   per-disk locking is unneeded. */
void
disk_read_internal (struct disk *d, disk_sector_t sec_no, void *buffer) 
{
  struct channel *c;
  
  ASSERT (d != NULL);
  ASSERT (buffer != NULL);

  c = d->channel;
  lock_acquire (&c->lock);
  select_sector (d, sec_no);
  issue_pio_command (c, CMD_READ_SECTOR_RETRY);
  sema_down (&c->completion_wait);
  if (!wait_while_busy (d))
    PANIC ("%s: disk read failed, sector=%"PRDSNu, d->name, sec_no);
  input_sector (c, buffer);
  d->read_cnt++;
  lock_release (&c->lock);
}





DRESULT
disk_read (
				BYTE Drive,          /* Physical drive number */
				PBYTE Buffer,        /* Pointer to the read data buffer */
				DWORD SectorNumber,  /* Start sector number */
				BYTE SectorCount     /* Number of sectros to read */
				)
{

	int chan_no = (Drive & 0x2) == 0x2;
	int dev_no = (Drive & 0x1) == 0x1;
	struct disk *d = NULL;
	int sec_no = 0;
	
	d = disk_get(chan_no,dev_no);

	ASSERT(d);

	for (sec_no = 0; sec_no < SectorCount; sec_no ++) {
		disk_read_internal (d, SectorNumber + sec_no, Buffer + (DISK_SECTOR_SIZE * sec_no)); 
	}

	return RES_OK;
}








/* Write sector SEC_NO to disk D from BUFFER, which must contain
   DISK_SECTOR_SIZE bytes.  Returns after the disk has
   acknowledged receiving the data.
   Internally synchronizes accesses to disks, so external
   per-disk locking is unneeded. */
void
disk_write_internal (struct disk *d, disk_sector_t sec_no, const void *buffer)
{
  struct channel *c;
  
  ASSERT (d != NULL);
  ASSERT (buffer != NULL);

  c = d->channel;
  lock_acquire (&c->lock);
  select_sector (d, sec_no);
  issue_pio_command (c, CMD_WRITE_SECTOR_RETRY);
  if (!wait_while_busy (d))
    PANIC ("%s: disk write failed, sector=%"PRDSNu, d->name, sec_no);
  output_sector (c, buffer);
  sema_down (&c->completion_wait);
  d->write_cnt++;
  lock_release (&c->lock);
}


DRESULT
disk_write (
				BYTE Drive,          /* Physical drive number */
				const PBYTE Buffer,  /* Pointer to the write data (may be non aligned) */
				DWORD SectorNumber,  /* Sector number to write */
				BYTE SectorCount     /* Number of sectors to write */
				)
{

	int chan_no = (Drive & 0x2) == 0x2;
	int dev_no = (Drive & 0x1) == 0x1;
	struct disk *d = NULL;
	int sec_no = 0;
	
	d = disk_get(chan_no,dev_no);

	ASSERT(d);

	for (sec_no = 0; sec_no < SectorCount; sec_no ++) {
		disk_write_internal (d, SectorNumber + sec_no, Buffer + (DISK_SECTOR_SIZE * sec_no)); 
	}

	return RES_OK;
}




/* Disk detection and identification. */

static void print_ata_string (char *string, size_t size);

/* Resets an ATA channel and waits for any devices present on it
   to finish the reset. */
static void
reset_channel (struct channel *c) 
{
  bool present[2];
  int dev_no;

  /* The ATA reset sequence depends on which devices are present,
     so we start by detecting device presence. */
  for (dev_no = 0; dev_no < 2; dev_no++)
    {
      struct disk *d = &c->devices[dev_no];

      select_device (d);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      outb (reg_nsect (c), 0xaa);
      outb (reg_lbal (c), 0x55);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      present[dev_no] = (inb (reg_nsect (c)) == 0x55
                         && inb (reg_lbal (c)) == 0xaa);
    }

  /* Issue soft reset sequence, which selects device 0 as a side effect.
     Also enable interrupts. */
  outb (reg_ctl (c), 0);
  timer_usleep (10);
  outb (reg_ctl (c), CTL_SRST);
  timer_usleep (10);
  outb (reg_ctl (c), 0);

  timer_msleep (150);

  /* Wait for device 0 to clear BSY. */
  if (present[0]) 
    {
      select_device (&c->devices[0]);
      wait_while_busy (&c->devices[0]); 
    }

  /* Wait for device 1 to clear BSY. */
  if (present[1])
    {
      int i;

      select_device (&c->devices[1]);
      for (i = 0; i < 3000; i++) 
        {
          if (inb (reg_nsect (c)) == 1 && inb (reg_lbal (c)) == 1)
            break;
          timer_msleep (10);
        }
      wait_while_busy (&c->devices[1]);
    }
}

/* Checks whether device D is an ATA disk and sets D's is_ata
   member appropriately.  If D is device 0 (master), returns true
   if it's possible that a slave (device 1) exists on this
   channel.  If D is device 1 (slave), the return value is not
   meaningful. */
static bool
check_device_type (struct disk *d) 
{
  struct channel *c = d->channel;
  uint8_t error, lbam, lbah, status;

  select_device (d);

  error = inb (reg_error (c));
  lbam = inb (reg_lbam (c));
  lbah = inb (reg_lbah (c));
  status = inb (reg_status (c));

  if ((error != 1 && (error != 0x81 || d->dev_no == 1))
      || (status & STA_DRDY) == 0
      || (status & STA_BSY) != 0)
    {
      d->is_ata = false;
      return error != 0x81;      
    }
  else 
    {
      d->is_ata = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
      return true; 
    }
}

/* Sends an IDENTIFY DEVICE command to disk D and reads the
   response.  Initializes D's capacity member based on the result
   and prints a message describing the disk to the console. */
static void
identify_ata_device (struct disk *d) 
{
  struct channel *c = d->channel;
  uint16_t id[DISK_SECTOR_SIZE / 2];

  ASSERT (d->is_ata);

  /* Send the IDENTIFY DEVICE command, wait for an interrupt
     indicating the device's response is ready, and read the data
     into our buffer. */
  select_device_wait (d);
  issue_pio_command (c, CMD_IDENTIFY_DEVICE);
  sema_down (&c->completion_wait);
  if (!wait_while_busy (d))
    {
      d->is_ata = false;
      return;
    }
  input_sector (c, id);

  /* Calculate capacity. */
  d->capacity = id[60] | ((uint32_t) id[61] << 16);

  /* Print identification message. */
  printf ("%s: detected %'"PRDSNu" sector (", d->name, d->capacity);
  if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024 * 1024)
    printf ("%"PRDSNu" GB",
            d->capacity / (1024 / DISK_SECTOR_SIZE * 1024 * 1024));
  else if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024)
    printf ("%"PRDSNu" MB", d->capacity / (1024 / DISK_SECTOR_SIZE * 1024));
  else if (d->capacity > 1024 / DISK_SECTOR_SIZE)
    printf ("%"PRDSNu" kB", d->capacity / (1024 / DISK_SECTOR_SIZE));
  else
    printf ("%"PRDSNu" byte", d->capacity * DISK_SECTOR_SIZE);
  printf (") disk, model \"");
  print_ata_string ((char *) &id[27], 40);
  printf ("\", serial \"");
  print_ata_string ((char *) &id[10], 20);
  printf ("\"\n");
}

/* Prints STRING, which consists of SIZE bytes in a funky format:
   each pair of bytes is in reverse order.  Does not print
   trailing whitespace and/or nulls. */
static void
print_ata_string (char *string, size_t size) 
{
  size_t i;

  /* Find the last non-white, non-null character. */
  for (; size > 0; size--)
    {
      int c = string[(size - 1) ^ 1];
      if (c != '\0' && !isspace (c))
        break; 
    }

  /* Print. */
  for (i = 0; i < size; i++)
    printf ("%c", string[i ^ 1]);
}

/* Selects device D, waiting for it to become ready, and then
   writes SEC_NO to the disk's sector selection registers.  (We
   use LBA mode.) */
static void
select_sector (struct disk *d, disk_sector_t sec_no) 
{
  struct channel *c = d->channel;

  ASSERT (sec_no < d->capacity);
  ASSERT (sec_no < (1UL << 28));
  
  select_device_wait (d);
  outb (reg_nsect (c), 1);
  outb (reg_lbal (c), sec_no);
  outb (reg_lbam (c), sec_no >> 8);
  outb (reg_lbah (c), (sec_no >> 16));
  outb (reg_device (c),
        DEV_MBS | DEV_LBA | (d->dev_no == 1 ? DEV_DEV : 0) | (sec_no >> 24));
}

/* Writes COMMAND to channel C and prepares for receiving a
   completion interrupt. */
static void
issue_pio_command (struct channel *c, uint8_t command) 
{
  /* Interrupts must be enabled or our semaphore will never be
     up'd by the completion handler. */
  ASSERT (intr_get_level () == INTR_ON);

  c->expecting_interrupt = true;
  outb (reg_command (c), command);
}

/* Reads a sector from channel C's data register in PIO mode into
   SECTOR, which must have room for DISK_SECTOR_SIZE bytes. */
static void
input_sector (struct channel *c, void *sector) 
{
  insw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* Writes SECTOR to channel C's data register in PIO mode.
   SECTOR must contain DISK_SECTOR_SIZE bytes. */
static void
output_sector (struct channel *c, const void *sector) 
{
  outsw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* Low-level ATA primitives. */

/* Wait up to 10 seconds for the controller to become idle, that
   is, for the BSY and DRQ bits to clear in the status register.

   As a side effect, reading the status register clears any
   pending interrupt. */
static void
wait_until_idle (const struct disk *d) 
{
  int i;

  for (i = 0; i < 1000; i++) 
    {
      if ((inb (reg_status (d->channel)) & (STA_BSY | STA_DRQ)) == 0)
        return;
      timer_usleep (10);
    }

  printf ("%s: idle timeout\n", d->name);
}

/* Wait up to 30 seconds for disk D to clear BSY,
   and then return the status of the DRQ bit.
   The ATA standards say that a disk may take as long as that to
   complete its reset. */
static bool
wait_while_busy (const struct disk *d) 
{
  struct channel *c = d->channel;
  int i;
  
  for (i = 0; i < 3000; i++)
    {
      if (i == 700)
        printf ("%s: busy, waiting...", d->name);
      if (!(inb (reg_alt_status (c)) & STA_BSY)) 
        {
          if (i >= 700)
            printf ("ok\n");
          return (inb (reg_alt_status (c)) & STA_DRQ) != 0;
        }
      timer_msleep (10);
    }

  printf ("failed\n");
  return false;
}

/* Program D's channel so that D is now the selected disk. */
static void
select_device (const struct disk *d)
{
  struct channel *c = d->channel;
  uint8_t dev = DEV_MBS;
  if (d->dev_no == 1)
    dev |= DEV_DEV;
  outb (reg_device (c), dev);
  inb (reg_alt_status (c));
  timer_nsleep (400);
}

/* Select disk D in its channel, as select_device(), but wait for
   the channel to become idle before and after. */
static void
select_device_wait (const struct disk *d) 
{
  wait_until_idle (d);
  select_device (d);
  wait_until_idle (d);
}

/* ATA interrupt handler. */
static void
interrupt_handler (struct intr_frame *f) 
{

	KeInsertQueueDpc(&IdeDpcObject, NULL, NULL);

#if 0
  struct channel *c;

  for (c = channels; c < channels + CHANNEL_CNT; c++)
    if (f->TrapFrame_trapno == c->irq)
      {
        if (c->expecting_interrupt) 
          {
            inb (reg_status (c));               /* Acknowledge interrupt. */
            sema_up (&c->completion_wait);      /* Wake up waiter. */
          }
        else
          printf ("%s: unexpected interrupt\n", c->name);
        return;
      }



  NOT_REACHED ();
  #endif 
}









DRESULT
disk_ioctl (
				BYTE Drive,      /* Drive number */
				BYTE Command,    /* Control command code */
				void* Buffer     /* Parameter and data buffer */
				)
{
	DRESULT Result = RES_OK;
	DWORD *Bufint;

	TRACEEXEC;
	ASSERT(Drive < 4);



	switch(Command) {

		case CTRL_SYNC:
			Result = RES_OK;
			break;


		case GET_SECTOR_COUNT:

		{
			int chan_no = (Drive & 0x2) == 0x2;
			int dev_no = (Drive & 0x1) == 0x1;
			struct disk *d = NULL;
			int sec_no = 0;
			
			d = disk_get(chan_no,dev_no);

			ASSERT(d);
		
			Bufint = (DWORD *)Buffer;
//			*Bufint= (64 * 1024 * 1024)/SECTOR_SIZE; //sector count for 64MB disk
			
			printf("GET_SECTOR_COUNT: %x\n", disk_size(d));
			*Bufint= disk_size(d);
			Result = RES_OK;
			break;
		}
		
		case GET_SECTOR_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint=SECTOR_SIZE;
			Result = RES_OK;
			break;

		case GET_BLOCK_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint = SECTOR_SIZE;
			Result = RES_OK;
			break;

		case CTRL_ERASE_SECTOR:
			Result = RES_OK;
			break;

		default:
			ASSERT(0);
			Result = RES_PARERR;
			break;
			

	}


	return Result;
}



DWORD get_fattime (void)
{
	TRACEEXEC;
	return 0;
}

DSTATUS
disk_initialize(BYTE drv)
{
	return 0;
}


DSTATUS 
disk_status (
							BYTE drv
							)
{
	return 0;

}



#endif


#if 0
#if _NEW_IDE_CODE_


#define timer_usleep(x)
#define timer_nsleep(x)
#define timer_msleep(x)

/* The code in this file is an interface to an ATA (IDE)
   controller.  It attempts to comply to [ATA-3]. */

/* ATA command block port addresses. */
#define reg_data(CHANNEL) ((CHANNEL)->reg_base + 0)     /* Data. */
#define reg_error(CHANNEL) ((CHANNEL)->reg_base + 1)    /* Error. */
#define reg_nsect(CHANNEL) ((CHANNEL)->reg_base + 2)    /* Sector Count. */
#define reg_lbal(CHANNEL) ((CHANNEL)->reg_base + 3)     /* LBA 0:7. */
#define reg_lbam(CHANNEL) ((CHANNEL)->reg_base + 4)     /* LBA 15:8. */
#define reg_lbah(CHANNEL) ((CHANNEL)->reg_base + 5)     /* LBA 23:16. */
#define reg_device(CHANNEL) ((CHANNEL)->reg_base + 6)   /* Device/LBA 27:24. */
#define reg_status(CHANNEL) ((CHANNEL)->reg_base + 7)   /* Status (r/o). */
#define reg_command(CHANNEL) reg_status (CHANNEL)       /* Command (w/o). */

/* ATA control block port addresses.
   (If we supported non-legacy ATA controllers this would not be
   flexible enough, but it's fine for what we do.) */
#define reg_ctl(CHANNEL) ((CHANNEL)->reg_base + 0x206)  /* Control (w/o). */
#define reg_alt_status(CHANNEL) reg_ctl (CHANNEL)       /* Alt Status (r/o). */

/* Alternate Status Register bits. */
#define STA_BSY 0x80            /* Busy. */
#define STA_DRDY 0x40           /* Device Ready. */
#define STA_DRQ 0x08            /* Data Request. */

/* Control Register bits. */
#define CTL_SRST 0x04           /* Software Reset. */

/* Device Register bits. */
#define DEV_MBS 0xa0            /* Must be set. */
#define DEV_LBA 0x40            /* Linear based addressing. */
#define DEV_DEV 0x10            /* Select device: 0=master, 1=slave. */

/* Commands.
   Many more are defined but this is the small subset that we
   use. */
#define CMD_IDENTIFY_DEVICE 0xec        /* IDENTIFY DEVICE. */
#define CMD_READ_SECTOR_RETRY 0x20      /* READ SECTOR with retries. */
#define CMD_WRITE_SECTOR_RETRY 0x30     /* WRITE SECTOR with retries. */

/* An ATA device. */
struct disk 
  {
    char name[8];               /* Name, e.g. "hd0:1". */
    struct channel *channel;    /* Channel disk is on. */
    int dev_no;                 /* Device 0 or 1 for master or slave. */

    BOOL is_ata;                /* 1=This device is an ATA disk. */
    ULONG capacity;     /* Capacity in sectors (if is_ata). */

    long long read_cnt;         /* Number of sectors read. */
    long long write_cnt;        /* Number of sectors written. */
  };

/* An ATA channel (aka controller).
   Each channel can control up to two disks. */
struct channel 
  {
    char name[8];               /* Name, e.g. "hd0". */
    uint16_t reg_base;          /* Base I/O port. */
    uint8_t irq;                /* Interrupt in use. */

    KMUTEX lock;           /* Must acquire to access the controller. */
    BOOL expecting_interrupt;   /* True if an interrupt is expected, false if
                                   any interrupt would be spurious. */
    KSEMAPHORE completion_wait;   /* Up'd by interrupt handler. */

    struct disk devices[2];     /* The devices on this channel. */
  };

KDPC IdeDpcObject;

/* We support the two "legacy" ATA channels found in a standard PC. */
#define CHANNEL_CNT 2
static struct channel channels[CHANNEL_CNT];

static void reset_channel (struct channel *);
static BOOL check_device_type (struct disk *);
static void identify_ata_device (struct disk *);

static void select_sector (struct disk *, ULONG);
static void issue_pio_command (struct channel *, uint8_t command);
static void input_sector (struct channel *, void *);
static void output_sector (struct channel *, const void *);

static void wait_until_idle (const struct disk *);
static BOOL wait_while_busy (const struct disk *);
static void select_device (const struct disk *);
static void select_device_wait (const struct disk *);

static void interrupt_handler (IN PKTRAPFRAME TrapFrame);




VOID
IdeDpcRoutine (
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	);





/* Initialize the disk subsystem and detect disks. */
void
disk_init (void) 
{
  size_t chan_no;

  for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++)
    {
      struct channel *c = &channels[chan_no];
      int dev_no;

      /* Initialize channel. */
      snprintf (c->name, sizeof c->name, "hd%zu", chan_no);
      switch (chan_no) 
        {
        case 0:
          c->reg_base = 0x1f0;
          c->irq = IrqPrimaryIDE; //14 + 0x20;
          break;
        case 1:
          c->reg_base = 0x170;
          c->irq = IrqSecondaryIDE; //15 + 0x20;
          break;
        default:
		KeBugCheck("Should Not Reach here\n");
        }

	if (HalAreInterruptsEnabled() == FALSE) {
		KeBugCheck("Interrupts Should be enabled \n");
	}
	
      //lock_init (&c->lock);
	KeInitializeMutant(&c->lock, FALSE);
      c->expecting_interrupt = FALSE;
      //sema_init (&c->completion_wait, 0);
	KeInitializeSemaphore(&c->completion_wait, 0, 100);
 
      /* Initialize devices. */
      for (dev_no = 0; dev_no < 2; dev_no++)
        {
          struct disk *d = &c->devices[dev_no];
          snprintf (d->name, sizeof d->name, "%s:%d", c->name, dev_no);
          d->channel = c;
          d->dev_no = dev_no;

          d->is_ata = FALSE;
          d->capacity = 0;

          d->read_cnt = d->write_cnt = 0;
        }

      /* Register interrupt handler. */
   //   intr_register_ext (c->irq, interrupt_handler, c->name);

	KiCreateInterruptObject(
		c->irq,
		0,
		interrupt_handler,
		c->name);


	KeInitializeDpc(&IdeDpcObject,
		&IdeDpcRoutine,NULL);

      /* Reset hardware. */
      reset_channel (c);

      /* Distinguish ATA hard disks from other devices. */
      if (check_device_type (&c->devices[0]))
        check_device_type (&c->devices[1]);

      /* Read hard disk identity information. */
      for (dev_no = 0; dev_no < 2; dev_no++)
        if (c->devices[dev_no].is_ata)
          identify_ata_device (&c->devices[dev_no]);
    }
}



DSTATUS
disk_initialize(BYTE drv)
{
	return 0;
}

DSTATUS 
disk_status (
							BYTE drv
							)
{
	return 0;

}

struct disk *
disk_get (int chan_no, int dev_no);


/* Prints disk statistics. */
void
disk_print_stats (void) 
{
  int chan_no;

  for (chan_no = 0; chan_no < CHANNEL_CNT; chan_no++) 
    {
      int dev_no;

      for (dev_no = 0; dev_no < 2; dev_no++) 
        {
          struct disk *d;

		d = disk_get (chan_no, dev_no);
          if (d != NULL && d->is_ata) 
            printf ("%s: %lld reads, %lld writes\n",
                    d->name, d->read_cnt, d->write_cnt);
        }
    }
}

/* Returns the disk numbered DEV_NO--either 0 or 1 for master or
   slave, respectively--within the channel numbered CHAN_NO.

   Pintos uses disks this way:
        0:0 - boot loader, command line args, and operating system kernel
        0:1 - file system
        1:0 - scratch
        1:1 - swap
*/
struct disk *
disk_get (int chan_no, int dev_no) 
{
  ASSERT (dev_no == 0 || dev_no == 1);

  if (chan_no < (int) CHANNEL_CNT) 
    {
      struct disk *d = &channels[chan_no].devices[dev_no];
      if (d->is_ata)
        return d; 
    }
  return NULL;
}

/* Returns the size of disk D, measured in DISK_SECTOR_SIZE-byte
   sectors. */
ULONG
disk_size (struct disk *d) 
{
  ASSERT (d != NULL);
  
  return d->capacity;
}

/* Reads sector SEC_NO from disk D into BUFFER, which must have
   room for DISK_SECTOR_SIZE bytes.
   Internally synchronizes accesses to disks, so external
   per-disk locking is unneeded. */

void
internal_disk_read (struct disk *d, ULONG sec_no, void *buffer) 
{
  struct channel *c;
  
  ASSERT (d != NULL);
  ASSERT (buffer != NULL);

  c = d->channel;
  //lock_acquire (&c->lock);
  printf("internal_disk_read: aquire c->lock\n");
  KeWaitForSingleObject(&c->lock,
  		Executive, KernelMode, FALSE,
  		NULL);
  
  select_sector (d, sec_no);
  issue_pio_command (c, CMD_READ_SECTOR_RETRY);
//  sema_down (&c->completion_wait);
  KeWaitForSingleObject(&c->completion_wait,
  		Executive, KernelMode, FALSE,
  		NULL);

  if (!wait_while_busy (d)) {
	KeBugCheck("%s: disk read failed, sector=%u", d->name, sec_no);
  }

  input_sector (c, buffer);
  d->read_cnt++;

//  lock_release (&c->lock);
  printf("internal_disk_read: release c->lock\n");
  KeReleaseMutant(&c->lock, 0, FALSE, FALSE);

}

DRESULT
disk_read (
				BYTE Drive,          /* Physical drive number */
				PBYTE Buffer,        /* Pointer to the read data buffer */
				DWORD SectorNumber,  /* Start sector number */
				BYTE SectorCount     /* Number of sectros to read */
				)
{

	int chan_no = (Drive & 0x2) == 0x2;
	int dev_no = (Drive & 0x1) == 0x1;
	struct disk *d = NULL;
	int sec_no = 0;
	
	d = disk_get(chan_no,dev_no);

	ASSERT(d);

	for (sec_no = 0; sec_no < SectorCount; sec_no ++) {
		internal_disk_read (d, SectorNumber + sec_no, Buffer + (DISK_SECTOR_SIZE * sec_no)); 
	}

	return RES_OK;
}




/* Write sector SEC_NO to disk D from BUFFER, which must contain
   DISK_SECTOR_SIZE bytes.  Returns after the disk has
   acknowledged receiving the data.
   Internally synchronizes accesses to disks, so external
   per-disk locking is unneeded. */
void
internal_disk_write (struct disk *d, ULONG sec_no, const void *buffer)
{
  struct channel *c;
  
  ASSERT (d != NULL);
  ASSERT (buffer != NULL);

  c = d->channel;
//  lock_acquire (&c->lock);

	printf("internal_disk_write: aquire c->lock\n");

  KeWaitForSingleObject(&c->lock,
  		Executive, KernelMode, FALSE,
  		NULL);
  

  select_sector (d, sec_no);
  issue_pio_command (c, CMD_WRITE_SECTOR_RETRY);
  if (!wait_while_busy (d))
    KeBugCheck ("%s: disk write failed, sector=%u", d->name, sec_no);
  output_sector (c, buffer);
  	printf("internal_disk_write: aquire semaphore\n");
//  sema_down (&c->completion_wait);
  KeWaitForSingleObject(&c->completion_wait,
  		Executive, KernelMode, FALSE,
  		NULL);

  d->write_cnt++;
//  lock_release (&c->lock);
	printf("internal_disk_write: Release c->lock\n");
	KeReleaseMutant(&c->lock, 0, FALSE, FALSE);

}


DRESULT
disk_write (
				BYTE Drive,          /* Physical drive number */
				const PBYTE Buffer,  /* Pointer to the write data (may be non aligned) */
				DWORD SectorNumber,  /* Sector number to write */
				BYTE SectorCount     /* Number of sectors to write */
				)
{

	int chan_no = (Drive & 0x2) == 0x2;
	int dev_no = (Drive & 0x1) == 0x1;
	struct disk *d = NULL;
	int sec_no = 0;
	
	d = disk_get(chan_no,dev_no);

	ASSERT(d);

	for (sec_no = 0; sec_no < SectorCount; sec_no ++) {
		internal_disk_write (d, SectorNumber + sec_no, Buffer + (DISK_SECTOR_SIZE * sec_no)); 
	}

	return RES_OK;
}





/* Disk detection and identification. */

static void print_ata_string (char *string, size_t size);

/* Resets an ATA channel and waits for any devices present on it
   to finish the reset. */
static void
reset_channel (struct channel *c) 
{
  BOOL present[2];
  int dev_no;

  /* The ATA reset sequence depends on which devices are present,
     so we start by detecting device presence. */
  for (dev_no = 0; dev_no < 2; dev_no++)
    {
      struct disk *d = &c->devices[dev_no];

      select_device (d);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      outb (reg_nsect (c), 0xaa);
      outb (reg_lbal (c), 0x55);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      present[dev_no] = (inb (reg_nsect (c)) == 0x55
                         && inb (reg_lbal (c)) == 0xaa);
    }

  /* Issue soft reset sequence, which selects device 0 as a side effect.
     Also enable interrupts. */
  outb (reg_ctl (c), 0);
  timer_usleep (10);
  outb (reg_ctl (c), CTL_SRST);
  timer_usleep (10);
  outb (reg_ctl (c), 0);

  timer_msleep (150);

  /* Wait for device 0 to clear BSY. */
  if (present[0]) 
    {
      select_device (&c->devices[0]);
      wait_while_busy (&c->devices[0]); 
    }

  /* Wait for device 1 to clear BSY. */
  if (present[1])
    {
      int i;

      select_device (&c->devices[1]);
      for (i = 0; i < 3000; i++) 
        {
          if (inb (reg_nsect (c)) == 1 && inb (reg_lbal (c)) == 1)
            break;
          timer_msleep (10);
        }
      wait_while_busy (&c->devices[1]);
    }
}

/* Checks whether device D is an ATA disk and sets D's is_ata
   member appropriately.  If D is device 0 (master), returns true
   if it's possible that a slave (device 1) exists on this
   channel.  If D is device 1 (slave), the return value is not
   meaningful. */
static BOOL
check_device_type (struct disk *d) 
{
  struct channel *c = d->channel;
  uint8_t error, lbam, lbah, status;

  select_device (d);

  error = inb (reg_error (c));
  lbam = inb (reg_lbam (c));
  lbah = inb (reg_lbah (c));
  status = inb (reg_status (c));

  if ((error != 1 && (error != 0x81 || d->dev_no == 1))
      || (status & STA_DRDY) == 0
      || (status & STA_BSY) != 0)
    {
      d->is_ata = FALSE;
      return error != 0x81;      
    }
  else 
    {
      d->is_ata = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
      return TRUE; 
    }
}

/* Sends an IDENTIFY DEVICE command to disk D and reads the
   response.  Initializes D's capacity member based on the result
   and prints a message describing the disk to the console. */
static void
identify_ata_device (struct disk *d) 
{
  struct channel *c = d->channel;
  uint16_t id[DISK_SECTOR_SIZE / 2];

  ASSERT (d->is_ata);

  /* Send the IDENTIFY DEVICE command, wait for an interrupt
     indicating the device's response is ready, and read the data
     into our buffer. */
  select_device_wait (d);
  issue_pio_command (c, CMD_IDENTIFY_DEVICE);
//  sema_down (&c->completion_wait);

  KeWaitForSingleObject(&c->completion_wait,
	  Executive, KernelMode, FALSE,
	  NULL);

  if (!wait_while_busy (d))
    {
      d->is_ata = FALSE;
      return;
    }
  input_sector (c, id);

  /* Calculate capacity. */
  d->capacity = id[60] | ((uint32_t) id[61] << 16);

  /* Print identification message. */
  printf ("%s: detected %u sector (", d->name, d->capacity);
  if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024 * 1024)
    printf ("%u GB",
            d->capacity / (1024 / DISK_SECTOR_SIZE * 1024 * 1024));
  else if (d->capacity > 1024 / DISK_SECTOR_SIZE * 1024)
    printf ("%u MB", d->capacity / (1024 / DISK_SECTOR_SIZE * 1024));
  else if (d->capacity > 1024 / DISK_SECTOR_SIZE)
    printf ("%u kB", d->capacity / (1024 / DISK_SECTOR_SIZE));
  else
    printf ("%u byte", d->capacity * DISK_SECTOR_SIZE);
  printf (") disk, model \"");
  print_ata_string ((char *) &id[27], 40);
  printf ("\", serial \"");
  print_ata_string ((char *) &id[10], 20);
  printf ("\"\n");
}

/* Prints STRING, which consists of SIZE bytes in a funky format:
   each pair of bytes is in reverse order.  Does not print
   trailing whitespace and/or nulls. */
static void
print_ata_string (char *string, size_t size) 
{
  size_t i;

  /* Find the last non-white, non-null character. */
  for (; size > 0; size--)
    {
      int c = string[(size - 1) ^ 1];
      if (c != '\0' && !isspace (c))
        break; 
    }

  /* Print. */
  for (i = 0; i < size; i++)
    printf ("%c", string[i ^ 1]);
}

/* Selects device D, waiting for it to become ready, and then
   writes SEC_NO to the disk's sector selection registers.  (We
   use LBA mode.) */
static void
select_sector (struct disk *d, ULONG sec_no) 
{
  struct channel *c = d->channel;

  ASSERT (sec_no < d->capacity);
  ASSERT (sec_no < (1UL << 28));
  
  select_device_wait (d);
  outb (reg_nsect (c), 1);
  outb (reg_lbal (c), sec_no);
  outb (reg_lbam (c), sec_no >> 8);
  outb (reg_lbah (c), (sec_no >> 16));
  outb (reg_device (c),
        DEV_MBS | DEV_LBA | (d->dev_no == 1 ? DEV_DEV : 0) | (sec_no >> 24));
}

/* Writes COMMAND to channel C and prepares for receiving a
   completion interrupt. */
static void
issue_pio_command (struct channel *c, uint8_t command) 
{
  /* Interrupts must be enabled or our semaphore will never be
     up'd by the completion handler. */
//  ASSERT (intr_get_level () == INTR_ON);

  c->expecting_interrupt = TRUE;
  outb (reg_command (c), command);
}

/* Reads a sector from channel C's data register in PIO mode into
   SECTOR, which must have room for DISK_SECTOR_SIZE bytes. */
static void
input_sector (struct channel *c, void *sector) 
{
  insw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* Writes SECTOR to channel C's data register in PIO mode.
   SECTOR must contain DISK_SECTOR_SIZE bytes. */
static void
output_sector (struct channel *c, const void *sector) 
{
  outsw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
}

/* Low-level ATA primitives. */

/* Wait up to 10 seconds for the controller to become idle, that
   is, for the BSY and DRQ bits to clear in the status register.

   As a side effect, reading the status register clears any
   pending interrupt. */
static void
wait_until_idle (const struct disk *d) 
{
  int i;

  for (i = 0; i < 1000; i++) 
    {
      if ((inb (reg_status (d->channel)) & (STA_BSY | STA_DRQ)) == 0)
        return;
//      timer_usleep (10);
    }

  printf ("%s: idle timeout\n", d->name);
}

/* Wait up to 30 seconds for disk D to clear BSY,
   and then return the status of the DRQ bit.
   The ATA standards say that a disk may take as long as that to
   complete its reset. */
static BOOL
wait_while_busy (const struct disk *d) 
{


  struct channel *c = d->channel;
	int i, j;
	
	for (i = 0; i < 3000; i++)
	  {
		if (i == 700)
		  printf ("%s: busy, waiting...", d->name);
		
		if (!(inb (reg_alt_status (c)) & STA_BSY)) 
		  {
			if (i >= 700)
			  printf ("ok\n");
			return (inb (reg_alt_status (c)) & STA_DRQ) != 0;
		  }
//		timer_msleep (10);
		for (j = 0; j < 100000; j ++);

	  }
  
	printf ("failed\n");
	return FALSE;


}

/* Program D's channel so that D is now the selected disk. */
static void
select_device (const struct disk *d)
{
  struct channel *c = d->channel;
  uint8_t dev = DEV_MBS;
  if (d->dev_no == 1)
    dev |= DEV_DEV;
  outb (reg_device (c), dev);
  inb (reg_alt_status (c));
  timer_nsleep (400);
}

/* Select disk D in its channel, as select_device(), but wait for
   the channel to become idle before and after. */
static void
select_device_wait (const struct disk *d) 
{
  wait_until_idle (d);
  select_device (d);
  wait_until_idle (d);
}

/* ATA interrupt handler. */
static void
interrupt_handler (IN PKTRAPFRAME TrapFrame) 
{

	/*
		Just Queue the DPC
	*/
	ASSERT(0);
	kprintf("Int_h\n\n");

		KeInsertQueueDpc(&IdeDpcObject, NULL, NULL);
}


VOID
IdeDpcRoutine (
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)
{

  struct channel *c;


  kprintf("Int_h\n\n");

  for (c = channels; c < channels + CHANNEL_CNT; c++)
//    if (TrapFrame->TrapFrame_trapno == c->irq)
      {
        if (c->expecting_interrupt) 
          {
//            inb (reg_status (c));               /* Acknowledge interrupt. */
//            sema_up (&c->completion_wait);      /* Wake up waiter. */
		KeReleaseSemaphore(&c->completion_wait,
								0, 1,FALSE);
          }
//        else
//          printf ("%s: unexpected interrupt\n", c->name);
//        return;
      }

ASSERT(0);







}





DRESULT
disk_ioctl (
				BYTE Drive,      /* Drive number */
				BYTE Command,    /* Control command code */
				void* Buffer     /* Parameter and data buffer */
				)
{
	DRESULT Result = RES_OK;
	DWORD *Bufint;

	TRACEEXEC;
	ASSERT(Drive < 4);



	switch(Command) {

		case CTRL_SYNC:
			Result = RES_OK;
			break;


		case GET_SECTOR_COUNT:

		{
			int chan_no = (Drive & 0x2) == 0x2;
			int dev_no = (Drive & 0x1) == 0x1;
			struct disk *d = NULL;
			int sec_no = 0;
			
			d = disk_get(chan_no,dev_no);

			ASSERT(d);
		
			Bufint = (DWORD *)Buffer;
//			*Bufint= (64 * 1024 * 1024)/SECTOR_SIZE; //sector count for 64MB disk
			
			printf("GET_SECTOR_COUNT: %x\n", disk_size(d));
			*Bufint= disk_size(d);
			Result = RES_OK;
			break;
		}
		
		case GET_SECTOR_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint=SECTOR_SIZE;
			Result = RES_OK;
			break;

		case GET_BLOCK_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint = SECTOR_SIZE;
			Result = RES_OK;
			break;

		case CTRL_ERASE_SECTOR:
			Result = RES_OK;
			break;

		default:
			ASSERT(0);
			Result = RES_PARERR;
			break;
			

	}


	return Result;
}



DWORD get_fattime (void)
{
	TRACEEXEC;
	return 0;
}










#else
#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01


#define MAX_DISK	2


DSTATUS IdeDisk[MAX_DISK];





DSTATUS
disk_initialize(BYTE drv)

{
	ASSERT(drv < MAX_DISK);
	TRACEEXEC;
	if (drv < MAX_DISK) {
		IdeDisk[drv] = 0;
		return IdeDisk[drv];
	}

	return STA_NODISK;
}


DSTATUS disk_status (
							BYTE drv
							)
{
	ASSERT(drv < MAX_DISK);
	TRACEEXEC;

	if (drv < MAX_DISK) {

		return IdeDisk[drv];
	}

	return STA_NODISK;
}


static int
ide_wait_ready(BOOL check_error)
{
	int r;

	while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
		/* do nothing */;

	if (check_error && (r & (IDE_DF|IDE_ERR)) != 0) {

//		return -1;
	}

	return 0;
}

BOOL
ide_probe_disk1(void)
{
	int r, x;
	TRACEEXEC;

	// wait for Device 0 to be ready
	ide_wait_ready(0);

	// switch to Device 1
	outb(0x1F6, 0xE0 | (1<<4));

	// check for Device 1 to be ready for a while
	for (x = 0; x < 1000 && (r = inb(0x1F7)) == 0; x++)
		/* do nothing */;

	// switch back to Device 0
	outb(0x1F6, 0xE0 | (0<<4));

	kprintf("Device 1 presence: %d\n", (x < 1000));
	return (x < 1000);
}


DRESULT
disk_read (
				BYTE Drive,          /* Physical drive number */
				PBYTE Buffer,        /* Pointer to the read data buffer */
				DWORD SectorNumber,  /* Start sector number */
				BYTE SectorCount     /* Number of sectros to read */
				)
{
	int r;
	PBYTE LocalBuff = Buffer;
	ASSERT1(SectorCount <= 256, SectorCount);
	TRACEEXEC;


	ide_wait_ready(0);

	outb(0x1F2, SectorCount);
	outb(0x1F3, SectorNumber & 0xFF);
	outb(0x1F4, (SectorNumber >> 8) & 0xFF);
	outb(0x1F5, (SectorNumber >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((Drive&1)<<4) | ((SectorNumber>>24)&0x0F));
	outb(0x1F7, 0x20);	// CMD 0x20 means read sector

	for (; SectorCount > 0; SectorCount--, Buffer += SECTOR_SIZE) {
		if ((r = ide_wait_ready(IDE_ERR)) < 0) {
			TRACEEXEC;
			return RES_ERROR;
		}
		insl(0x1F0, Buffer, SECTOR_SIZE/4);
	}
	
	TRACEEXEC;
	return RES_OK;
}





DRESULT
disk_write (
				BYTE Drive,          /* Physical drive number */
				const PBYTE Buffer,  /* Pointer to the write data (may be non aligned) */
				DWORD SectorNumber,  /* Sector number to write */
				BYTE SectorCount     /* Number of sectors to write */
				)
{
	int r;
	PBYTE LocalBuff = Buffer;
	
	ASSERT1(SectorCount <= 256, SectorCount);
	TRACEEXEC;

	ide_wait_ready(0);

	outb(0x1F2, SectorCount);
	outb(0x1F3, SectorNumber & 0xFF);
	outb(0x1F4, (SectorNumber >> 8) & 0xFF);
	outb(0x1F5, (SectorNumber >> 16) & 0xFF);
	outb(0x1F6, 0xE0 | ((Drive &1)<<4) | ((SectorNumber >>24)&0x0F));
	outb(0x1F7, 0x30);	// CMD 0x30 means write sector

	for (; SectorCount > 0; SectorCount--, LocalBuff += SECTOR_SIZE) {
		if ((r = ide_wait_ready(1)) < 0)
			return r;
		outsl(0x1F0, LocalBuff, SECTOR_SIZE/4);
	}

	return 0;
}

/* Dummy function */
void
disk_init (void) 
{
}

DRESULT
disk_ioctl (
				BYTE Drive,      /* Drive number */
				BYTE Command,    /* Control command code */
				void* Buffer     /* Parameter and data buffer */
				)
{
	DRESULT Result = RES_OK;
	DWORD *Bufint;

	TRACEEXEC;

	if (Drive >= MAX_DISK) {

		return RES_PARERR;
	}


	switch(Command) {

		case CTRL_SYNC:
			Result = RES_OK;
			break;


		case GET_SECTOR_COUNT:
			Bufint = (DWORD *)Buffer;
			*Bufint= (64 * 1024 * 1024)/SECTOR_SIZE; //sector count for 64MB disk
			Result = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint=SECTOR_SIZE;
			Result = RES_OK;
			break;

		case GET_BLOCK_SIZE:
			Bufint = (DWORD *)Buffer;
			*Bufint = BLOCK_SIZE/SECTOR_SIZE;
			Result = RES_OK;
			break;

		case CTRL_ERASE_SECTOR:
			Result = RES_OK;
			break;

		default:
			ASSERT(0);
			Result = RES_PARERR;
			break;
			

	}


	return Result;
}



DWORD get_fattime (void)
{
	TRACEEXEC;
	return 0;
}

#endif



#endif

