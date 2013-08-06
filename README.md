tinyos
======

### A Quite Simple Operating System for education

At first, I have developed this tiny OS as Type-I Virtual Machine
Monitor, however I can not finish to run Linux operating system as
Guest OS.  The tiny OS has not been published a long time.  Recently,
I find out that we use the tiny OS for education because of the tiny
OS consists of a simple boot loader, basic memory allocator, and basic
device drivers (such as NE2000 driver). Current operating systems such
as Linux have very complicated mechanisms, thus students can not hack
these operating system casually.

### This Operating System Funtions
- First fit memory allocator
- Using segmentation for memory management
- basic device drivers (NE2000 compatible ethernet card, ATA/ATAPI device driver, PCI/ISA interface, and keyboard)
- and a basic library like glibc

### How to use
Install nasm, gcc, ld, make, and qemu
`$ apt-get install nasm gcc build-essential make qemu`
Next
`$ make`
Finally
`$ make test`
to run the tiny OS on qemu
