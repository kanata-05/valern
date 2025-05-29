# valern
***megalomaniacal***  

> [!IMPORTANT]
> Valern is under active development and currently not suitable for real-world use  
> (It can't even accept keyboard input)

# Building

### Prerequisites
Linux/Unix Enviroment (WSL2 works just fine)  
Virtualizer or real hardware (QEMU, Virtualbox, x86_64 compatible hardware)

### make
Just simply run `make` to create the kernel binaries which'll then call `build.sh`  
to get the limine bootloader, which'll then call `build_ISO.sh` to create a bootable  
ISO that you can use.  

# Information

## Kernel Status
Currently, the following have been implemented into the kernel:  

--WORKING--
Basic standard C ops
Basic memory management ops  
PSF font support  
GDT

--PARTIAL / BUG--
Shell
Interrupts System for keyboard
Keyboard Driver
Keyboard Input

  
TODO:  
Bash shell  
Filesystem (FAT32)  
  
However, the TODO list will shrink and grow as the project goes on.  

## Project
> ### what
valern is a small operating system   
it's built in C and a teeny tiny bit of assembly  
it's using the limine bootloader and protocol  

> ### why
for ~~fun~~ torture, and learning! (also because i can)    
i like how weirdly complex bootloaders and kernels are,  
so why not make one myself?  

> ### how
what do you mean how?  
...since bootloaders were quite annoying,  
i decided to use limine, i didn't need to write a whole lot of assembly after that.
i decided to use C to write the kernel, it's good enough for what i'm doing.
