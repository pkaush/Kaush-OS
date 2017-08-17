# Kaush-OS

This is the new home for this tiny OS. It has moved from the old SourceForge location(http://kosjaguar.sf.net/) to here on GitHub.

The following text is from the 8-year-old site. I will add in more details soon.


## Introduction

The Idea is to develop a modular OS for learning, research, and training purpose. This is being developed, keeping the interdependency of each module coherent, so that, to try out an idea in one area of the OS doesn't need to deal with other areas.

The internal design of the kernel might look similar to Windows Kernel, to some extent (because of the use of IRQLs and object manager). 

## Current Status

Following is the status of the development. This list might not get updated with every check in, but we will try to keep it updated with each major release.

1. Boot Loader.

2. Memory Manager - Support flat 4 GB virtual address space, 2GB Kernel, 2GB User.

3. Process and threads - The kernel is designed to be multi-thread safe. Threads can preempt in Kernel.

4. Interrupts - IDT and interrupt assignment is done. Currently, we are using PIC for interrupts but soon will shift to APIC for MP.

5. IRQLs - We are using our interrupts priority levels much similar to Windows using PIC masking. See more details under kernel section.

6. DPC - Per processor DPC queues are ready. It is being used by Scheduler. The concept is similar to Bottom Half of Linux.

7. MP - Our implementation is mostly ready for MP, the only thing blocking us to go there is PIC. We will implement APIC handling module and start out MP.

8. Spinlock - Spinlocks are acquired at DISPATCH_LEVEL and higher. For a UP version, we just raise the IRQL to DISPATH_LEVEL.

9. Wait Locks - Mutants, Semaphore, events and other kernel Objects which are based on DISPATCHER_HEADER.

10. The Scheduler is Round robin, priority based scheduler - not yet fully implemented but works great.

11. Object manager is implemented to provide a central control place for all the resourced being used. Every resource in our OS is an Object, like - Process, Threads, Mutex, Semaphore, Shared Memory, device, driver etc. Object manager is also responsible for providing access to FS and devices. FS can also be mounted on some path using Object manager.