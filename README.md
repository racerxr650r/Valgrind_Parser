# Valgrind_Parser
Simple command line utility to parse a Valgrind log file and present it in a more human readable format with the offending source code.

You can view the [Software Design Document](https://github.com/racerxr650r/Valgrind_Parser/blob/develop/SDD.md) here.

## Download this repository
```console
sudo apt update
sudo apt install git
git clone https://github.com/racerxr650r/Valgrind_Parser
```

## Build the application
```console
cd Valgrind_Parser
make
```
Change to the Valgrind_Parser directory and build the application.

## Install the application
```console
make install
```
This will install the application binary and the man page file to common paths. This will require sudo permission so you will be
prompted for your password.

> [!NOTE]
> BINDIR (directory to install the binary file) and MANDIR 
(directory to install the man page documentation) can be defined from the 
make command line. This will replace the default values. The defaults for 
BINDIR and MANDIR should work for most Linux distributions.

## Run the application
```console
valgrind --leak-check=full --log-file=valgrind-out.txt --fullpath-after=string ./your_program_to_test
vgp valgrind-out.txt
```
First run your application under test with Valgrind exporting the debug information to a log file named valgrind-out.txt. Then run
the Valgrind parser application (vgp) to parse the log file in human readable format with offending source code included.

> [!NOTE]
> When you build your application use the -g or -g3 debug info option and provide full pathnames for the source files. This will
> store the full pathnames along with the other debug information. To see an example of how to include the full pathnames for your
> source files see the makefile for vgp. When you run Valgrind be sure to include the --fullpath-after=string option so Valgrind
> outputs the full pathnames from the debug information. Doing this will insure that *vgp* can find the source files to display
> the offending function and line.

## Get usage information
```console
man vgp
```
man only works if you have installed vgp.

## Command line usage
```
USAGE: vgp <Valgrind_log_file>

The log filename is required.
```

## Uninstall vgp
```console
make uninstall
```
Uninstalls the vgp application and documentation.

## Example
The following is a typical sample of a Valgrind log file.
```
==4061210== Memcheck, a memory error detector
==4061210== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==4061210== Using Valgrind-3.19.0 and LibVEX; rerun with -h for copyright info
==4061210== Command: ./DnD
==4061210== Parent PID: 4035889
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10B722: strInputMethod (/home/john/Projects/DnD_App/comp.c:410)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73660 is 32 bytes inside a block of size 192 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10A8B0: compDestroyMethod (/home/john/Projects/DnD_App/comp.c:92)
==4061210==    by 0x111583: compDestroy (/home/john/Projects/DnD_App/lcaf.h:784)
==4061210==    by 0x1120A5: destroy_window_method (/home/john/Projects/DnD_App/win.c:342)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10B900: strCreate (/home/john/Projects/DnD_App/comp.c:449)
==4061210==    by 0x10F310: popupGetString (/home/john/Projects/DnD_App/popup.c:94)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10B726: strInputMethod (/home/john/Projects/DnD_App/comp.c:410)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b732a8 is 216 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10B731: strInputMethod (/home/john/Projects/DnD_App/comp.c:410)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73660 is 32 bytes inside a block of size 192 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10A8B0: compDestroyMethod (/home/john/Projects/DnD_App/comp.c:92)
==4061210==    by 0x111583: compDestroy (/home/john/Projects/DnD_App/lcaf.h:784)
==4061210==    by 0x1120A5: destroy_window_method (/home/john/Projects/DnD_App/win.c:342)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10B900: strCreate (/home/john/Projects/DnD_App/comp.c:449)
==4061210==    by 0x10F310: popupGetString (/home/john/Projects/DnD_App/popup.c:94)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x1121FC: next_focus_window (/home/john/Projects/DnD_App/win.c:391)
==4061210==    by 0x10B739: strInputMethod (/home/john/Projects/DnD_App/comp.c:410)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73208 is 56 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10B818: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73660 is 32 bytes inside a block of size 192 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10A8B0: compDestroyMethod (/home/john/Projects/DnD_App/comp.c:92)
==4061210==    by 0x111583: compDestroy (/home/john/Projects/DnD_App/lcaf.h:784)
==4061210==    by 0x1120A5: destroy_window_method (/home/john/Projects/DnD_App/win.c:342)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x10B900: strCreate (/home/john/Projects/DnD_App/comp.c:449)
==4061210==    by 0x10F310: popupGetString (/home/john/Projects/DnD_App/popup.c:94)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10A49C: winSetStale (/home/john/Projects/DnD_App/lcaf.h:373)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73290 is 192 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x10A4AC: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73290 is 192 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x111FE2: stale_window_method (/home/john/Projects/DnD_App/win.c:311)
==4061210==    by 0x10A4BB: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73220 is 80 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x111FEF: stale_window_method (/home/john/Projects/DnD_App/win.c:312)
==4061210==    by 0x10A4BB: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73220 is 80 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid write of size 1
==4061210==    at 0x111FFD: stale_window_method (/home/john/Projects/DnD_App/win.c:317)
==4061210==    by 0x10A4BB: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b731fe is 46 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x112005: stale_window_method (/home/john/Projects/DnD_App/win.c:320)
==4061210==    by 0x10A4BB: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73310 is 320 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== Invalid read of size 8
==4061210==    at 0x112029: stale_window_method (/home/john/Projects/DnD_App/win.c:323)
==4061210==    by 0x10A4BB: winSetStale (/home/john/Projects/DnD_App/lcaf.h:374)
==4061210==    by 0x10B823: strInputMethod (/home/john/Projects/DnD_App/comp.c:429)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Address 0x4b73230 is 96 bytes inside a block of size 352 free'd
==4061210==    at 0x484417B: free (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x1120FC: destroy_window_method (/home/john/Projects/DnD_App/win.c:355)
==4061210==    by 0x10A5EA: winDestroy (/home/john/Projects/DnD_App/lcaf.h:576)
==4061210==    by 0x10AD9D: listAction (/home/john/Projects/DnD_App/comp.c:205)
==4061210==    by 0x10B71D: strInputMethod (/home/john/Projects/DnD_App/comp.c:409)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210==  Block was alloc'd at
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x11299E: winAllocate (/home/john/Projects/DnD_App/win.c:701)
==4061210==    by 0x112D4A: winCreate (/home/john/Projects/DnD_App/win.c:778)
==4061210==    by 0x10F2BE: popupGetString (/home/john/Projects/DnD_App/popup.c:89)
==4061210==    by 0x10AEE3: listInputMethod (/home/john/Projects/DnD_App/comp.c:226)
==4061210==    by 0x111C0D: input_window (/home/john/Projects/DnD_App/win.c:176)
==4061210==    by 0x10FAEE: scrnRun (/home/john/Projects/DnD_App/scrn.c:257)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 
==4061210== HEAP SUMMARY:
==4061210==     in use at exit: 607,709 bytes in 571 blocks
==4061210==   total heap usage: 1,205 allocs, 634 frees, 717,162 bytes allocated
==4061210== 
==4061210== 9 bytes in 1 blocks are possibly lost in loss record 7 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x4972999: strdup (/./string/strdup.c:42)
==4061210==    by 0x48B81D5: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x487E52B: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487EF08: newterm_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487F1D7: newterm (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487ACEF: initscr (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F71C: scrnCreate (/home/john/Projects/DnD_App/scrn.c:129)
==4061210==    by 0x10C465: main (/home/john/Projects/DnD_App/DnD.c:70)
==4061210== 
==4061210== 24 bytes in 1 blocks are possibly lost in loss record 28 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x49D6D95: tsearch (misc/./misc/tsearch.c:337)
==4061210==    by 0x48B81F0: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x487E52B: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487EF08: newterm_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487F1D7: newterm (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487ACEF: initscr (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F71C: scrnCreate (/home/john/Projects/DnD_App/scrn.c:129)
==4061210==    by 0x10C465: main (/home/john/Projects/DnD_App/DnD.c:70)
==4061210== 
==4061210== 24 bytes in 1 blocks are possibly lost in loss record 29 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x49D6D95: tsearch (misc/./misc/tsearch.c:337)
==4061210==    by 0x48B81F0: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x4875839: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B49: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 24 bytes in 1 blocks are possibly lost in loss record 30 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x49D6D95: tsearch (misc/./misc/tsearch.c:337)
==4061210==    by 0x48B81F0: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48758B9: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B93: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 63 bytes in 1 blocks are possibly lost in loss record 35 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x4972999: strdup (/./string/strdup.c:42)
==4061210==    by 0x48B81D5: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x4875839: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B49: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 64 bytes in 1 blocks are possibly lost in loss record 36 of 88
==4061210==    at 0x48417B4: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x4972999: strdup (/./string/strdup.c:42)
==4061210==    by 0x48B81D5: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48758B9: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B93: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 168 bytes in 1 blocks are possibly lost in loss record 54 of 88
==4061210==    at 0x48465EF: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x48B814E: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x487E52B: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487EF08: newterm_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487F1D7: newterm (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x487ACEF: initscr (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F71C: scrnCreate (/home/john/Projects/DnD_App/scrn.c:129)
==4061210==    by 0x10C465: main (/home/john/Projects/DnD_App/DnD.c:70)
==4061210== 
==4061210== 168 bytes in 1 blocks are possibly lost in loss record 55 of 88
==4061210==    at 0x48465EF: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x48B814E: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x4875839: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B49: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== 168 bytes in 1 blocks are possibly lost in loss record 56 of 88
==4061210==    at 0x48465EF: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==4061210==    by 0x48B814E: ??? (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48BB2C0: _nc_tiparm (in /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4)
==4061210==    by 0x48758B9: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4876B93: ??? (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x488FDCB: doupdate_sp (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x4880F33: wrefresh (in /usr/lib/x86_64-linux-gnu/libncursesw.so.6.4)
==4061210==    by 0x10F6FA: scrnWriteMethod (/home/john/Projects/DnD_App/scrn.c:117)
==4061210==    by 0x10F426: scrnWrite (/home/john/Projects/DnD_App/lcaf.h:1060)
==4061210==    by 0x10F8FF: scrnRun (/home/john/Projects/DnD_App/scrn.c:199)
==4061210==    by 0x10C4B2: main (/home/john/Projects/DnD_App/DnD.c:81)
==4061210== 
==4061210== LEAK SUMMARY:
==4061210==    definitely lost: 0 bytes in 0 blocks
==4061210==    indirectly lost: 0 bytes in 0 blocks
==4061210==      possibly lost: 712 bytes in 9 blocks
==4061210==    still reachable: 606,997 bytes in 562 blocks
==4061210==         suppressed: 0 bytes in 0 blocks
==4061210== Reachable blocks (those to which a pointer was found) are not shown.
==4061210== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==4061210== 
==4061210== For lists of detected and suppressed errors, rerun with: -s
==4061210== ERROR SUMMARY: 21 errors from 21 contexts (suppressed: 0 from 0)
```

This is the vgp output parsed from the Valgrind log file above.

```
--- Valgrind Log Summary ---
Parsing Log File: valgrind-out.txt
----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - strInputMethod(comp.c:410)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/comp.c):
 373 int strInputMethod(Component *base, int ch)
 374 {
 375     String *str = (String *)base;
 376 
 377     switch(ch)
 378     {
 379         case KEY_HOME:
 380             str->cursor_offset = 0;
 381             break;
 382         case KEY_END:
 383             str->cursor_offset = strlen(str->value);
 384             break;
 385         case KEY_LEFT:
 386             if(str->cursor_offset > 0)
 387                 --str->cursor_offset;
 388             break;
 389         case KEY_RIGHT:
 390             if(str->cursor_offset < strlen(str->value))
 391                 ++str->cursor_offset;
 392             break;
 393         case KEY_BACKSPACE:
 394         if (str->cursor_offset > 0)
 395         {
 396             memmove(&str->value[str->cursor_offset - 1], &str->value[st... 
 397             --str->cursor_offset;
 398         }
 399         break;
 400         case KEY_DC:
 401             if(str->cursor_offset < strlen(str->value))
 402                 memmove(&str->value[str->cursor_offset],&str->value[str... 
 403             break;
 404         case '\n':
 405         case '\r':
 406         case KEY_ENTER:
 407         case KEY_ESC:
 408             if(base->notify_action != NULL)
 409                 base->notify_action(base, ch);
<410>            base->parent->next_focus(base->parent);
 411             break;
 412         default:
 413             if(isprint(ch))
 414             {
 415                 if(str->cursor_offset < str->size_max)
 416                 {
 417                     if(base->parent->insert_key)
 418                         strcpy(&str->value[str->cursor_offset+1],&str->... 
 419                     str->value[str->cursor_offset++] = ch;
 420                 }
 421             }
 422             else
 423             {
 424                 // Did not consume the input
 425                 return(0);
 426             }
 427     }
 428     // Consumed the input
 429     winSetStale(base->parent);
 430     return(1);
 431 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - strInputMethod(comp.c:410)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/comp.c):
 373 int strInputMethod(Component *base, int ch)
 374 {
 375     String *str = (String *)base;
 376 
 377     switch(ch)
 378     {
 379         case KEY_HOME:
 380             str->cursor_offset = 0;
 381             break;
 382         case KEY_END:
 383             str->cursor_offset = strlen(str->value);
 384             break;
 385         case KEY_LEFT:
 386             if(str->cursor_offset > 0)
 387                 --str->cursor_offset;
 388             break;
 389         case KEY_RIGHT:
 390             if(str->cursor_offset < strlen(str->value))
 391                 ++str->cursor_offset;
 392             break;
 393         case KEY_BACKSPACE:
 394         if (str->cursor_offset > 0)
 395         {
 396             memmove(&str->value[str->cursor_offset - 1], &str->value[st... 
 397             --str->cursor_offset;
 398         }
 399         break;
 400         case KEY_DC:
 401             if(str->cursor_offset < strlen(str->value))
 402                 memmove(&str->value[str->cursor_offset],&str->value[str... 
 403             break;
 404         case '\n':
 405         case '\r':
 406         case KEY_ENTER:
 407         case KEY_ESC:
 408             if(base->notify_action != NULL)
 409                 base->notify_action(base, ch);
<410>            base->parent->next_focus(base->parent);
 411             break;
 412         default:
 413             if(isprint(ch))
 414             {
 415                 if(str->cursor_offset < str->size_max)
 416                 {
 417                     if(base->parent->insert_key)
 418                         strcpy(&str->value[str->cursor_offset+1],&str->... 
 419                     str->value[str->cursor_offset++] = ch;
 420                 }
 421             }
 422             else
 423             {
 424                 // Did not consume the input
 425                 return(0);
 426             }
 427     }
 428     // Consumed the input
 429     winSetStale(base->parent);
 430     return(1);
 431 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - strInputMethod(comp.c:410)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/comp.c):
 373 int strInputMethod(Component *base, int ch)
 374 {
 375     String *str = (String *)base;
 376 
 377     switch(ch)
 378     {
 379         case KEY_HOME:
 380             str->cursor_offset = 0;
 381             break;
 382         case KEY_END:
 383             str->cursor_offset = strlen(str->value);
 384             break;
 385         case KEY_LEFT:
 386             if(str->cursor_offset > 0)
 387                 --str->cursor_offset;
 388             break;
 389         case KEY_RIGHT:
 390             if(str->cursor_offset < strlen(str->value))
 391                 ++str->cursor_offset;
 392             break;
 393         case KEY_BACKSPACE:
 394         if (str->cursor_offset > 0)
 395         {
 396             memmove(&str->value[str->cursor_offset - 1], &str->value[st... 
 397             --str->cursor_offset;
 398         }
 399         break;
 400         case KEY_DC:
 401             if(str->cursor_offset < strlen(str->value))
 402                 memmove(&str->value[str->cursor_offset],&str->value[str... 
 403             break;
 404         case '\n':
 405         case '\r':
 406         case KEY_ENTER:
 407         case KEY_ESC:
 408             if(base->notify_action != NULL)
 409                 base->notify_action(base, ch);
<410>            base->parent->next_focus(base->parent);
 411             break;
 412         default:
 413             if(isprint(ch))
 414             {
 415                 if(str->cursor_offset < str->size_max)
 416                 {
 417                     if(base->parent->insert_key)
 418                         strcpy(&str->value[str->cursor_offset+1],&str->... 
 419                     str->value[str->cursor_offset++] = ch;
 420                 }
 421             }
 422             else
 423             {
 424                 // Did not consume the input
 425                 return(0);
 426             }
 427     }
 428     // Consumed the input
 429     winSetStale(base->parent);
 430     return(1);
 431 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - next_focus_window(win.c:391)
  - strInputMethod(comp.c:410)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 385 void next_focus_window(Window *this)
 386 {
 387     if(this == NULL)
 388         return;
 389 
 390     // If there are no components for this window...
<391>    if(this->component_head == NULL || this->component_tail == NULL)
 392         return;
 393 
 394     // Find the next valid component
 395     Component *curr;
 396     // If there is a current focus...
 397     if(curr = this->focus)
 398     {
 399         // While there is a next component in the list...
 400         while(curr = curr->next)
 401         {
 402             // If this component can handle focus...
 403             if(curr->focus)
 404             {
 405                 winSetFocus(this,curr);
 406                 //this->focus = curr;
 407                 //curr->focus(curr);
 408                 //set_stale_window(this);
 409                 return;
 410             }
 411         }
 412     }
 413 
 414     curr = this->component_head;
 415 
 416     // Iterate through the list of components from head to tail
 417     do
 418     {
 419         // If component can handle focus...
 420         if(curr->focus)
 421         {
 422             winSetFocus(this,curr);
 423             //this->focus = curr;
 424             //curr->focus(curr);
 425             //set_stale_window(this);
 426             return;
 427         }
 428     }while(curr = curr->next);
 429 
 430     // Else there are no valid components
 431     this->focus = NULL;
 432 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/comp.c):
 373 int strInputMethod(Component *base, int ch)
 374 {
 375     String *str = (String *)base;
 376 
 377     switch(ch)
 378     {
 379         case KEY_HOME:
 380             str->cursor_offset = 0;
 381             break;
 382         case KEY_END:
 383             str->cursor_offset = strlen(str->value);
 384             break;
 385         case KEY_LEFT:
 386             if(str->cursor_offset > 0)
 387                 --str->cursor_offset;
 388             break;
 389         case KEY_RIGHT:
 390             if(str->cursor_offset < strlen(str->value))
 391                 ++str->cursor_offset;
 392             break;
 393         case KEY_BACKSPACE:
 394         if (str->cursor_offset > 0)
 395         {
 396             memmove(&str->value[str->cursor_offset - 1], &str->value[st... 
 397             --str->cursor_offset;
 398         }
 399         break;
 400         case KEY_DC:
 401             if(str->cursor_offset < strlen(str->value))
 402                 memmove(&str->value[str->cursor_offset],&str->value[str... 
 403             break;
 404         case '\n':
 405         case '\r':
 406         case KEY_ENTER:
 407         case KEY_ESC:
 408             if(base->notify_action != NULL)
 409                 base->notify_action(base, ch);
 410             base->parent->next_focus(base->parent);
 411             break;
 412         default:
 413             if(isprint(ch))
 414             {
 415                 if(str->cursor_offset < str->size_max)
 416                 {
 417                     if(base->parent->insert_key)
 418                         strcpy(&str->value[str->cursor_offset+1],&str->... 
 419                     str->value[str->cursor_offset++] = ch;
 420                 }
 421             }
 422             else
 423             {
 424                 // Did not consume the input
 425                 return(0);
 426             }
 427     }
 428     // Consumed the input
<429>    winSetStale(base->parent);
 430     return(1);
 431 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - winSetStale(lcaf.h:373)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/lcaf.h):
 370 static inline void winSetStale(Window *this)
 371 {
 372     if(this != NULL)
<373>        if(this->set_stale != NULL)
 374             this->set_stale(this);
 375     return;
 376 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/lcaf.h):
 370 static inline void winSetStale(Window *this)
 371 {
 372     if(this != NULL)
 373         if(this->set_stale != NULL)
<374>            this->set_stale(this);
 375     return;
 376 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - stale_window_method(win.c:311)
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 305 void stale_window_method(Window *this)
 306 {
 307     if(this == NULL)
 308         return;
 309 
 310     // Mark the associated screen stale
<311>    if(this->screen)
 312         this->screen->stale = true;
 313 
 314     // Mark this window and all of them above it stale
 315     while(this)
 316     {
 317         this->stale = true;
 318         
 319         // If this window has a stale notification...
 320         if(this->notify_stale)
 321             this->notify_stale(this);
 322 
 323         this = this->next;
 324     }
 325 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - stale_window_method(win.c:312)
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 305 void stale_window_method(Window *this)
 306 {
 307     if(this == NULL)
 308         return;
 309 
 310     // Mark the associated screen stale
 311     if(this->screen)
<312>        this->screen->stale = true;
 313 
 314     // Mark this window and all of them above it stale
 315     while(this)
 316     {
 317         this->stale = true;
 318         
 319         // If this window has a stale notification...
 320         if(this->notify_stale)
 321             this->notify_stale(this);
 322 
 323         this = this->next;
 324     }
 325 }

----------------------------------------
[ERROR] Invalid write of size 1
----------------------------------------
Call Stack:
  - stale_window_method(win.c:317)
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 305 void stale_window_method(Window *this)
 306 {
 307     if(this == NULL)
 308         return;
 309 
 310     // Mark the associated screen stale
 311     if(this->screen)
 312         this->screen->stale = true;
 313 
 314     // Mark this window and all of them above it stale
 315     while(this)
 316     {
<317>        this->stale = true;
 318         
 319         // If this window has a stale notification...
 320         if(this->notify_stale)
 321             this->notify_stale(this);
 322 
 323         this = this->next;
 324     }
 325 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - stale_window_method(win.c:320)
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 305 void stale_window_method(Window *this)
 306 {
 307     if(this == NULL)
 308         return;
 309 
 310     // Mark the associated screen stale
 311     if(this->screen)
 312         this->screen->stale = true;
 313 
 314     // Mark this window and all of them above it stale
 315     while(this)
 316     {
 317         this->stale = true;
 318         
 319         // If this window has a stale notification...
<320>        if(this->notify_stale)
 321             this->notify_stale(this);
 322 
 323         this = this->next;
 324     }
 325 }

----------------------------------------
[ERROR] Invalid read of size 8
----------------------------------------
Call Stack:
  - stale_window_method(win.c:323)
  - winSetStale(lcaf.h:374)
  - strInputMethod(comp.c:429)
  - input_window(win.c:176)
  - scrnRun(scrn.c:257)
  - main(DnD.c:81)
Source (/home/john/Projects/DnD_App/win.c):
 305 void stale_window_method(Window *this)
 306 {
 307     if(this == NULL)
 308         return;
 309 
 310     // Mark the associated screen stale
 311     if(this->screen)
 312         this->screen->stale = true;
 313 
 314     // Mark this window and all of them above it stale
 315     while(this)
 316     {
 317         this->stale = true;
 318         
 319         // If this window has a stale notification...
 320         if(this->notify_stale)
 321             this->notify_stale(this);
 322 
<323>        this = this->next;
 324     }
 325 }


--- LEAK SUMMARY ---
* Definitely Lost: 0 bytes in 0 blocks
* Indirectly Lost: 0 bytes in 0 blocks
* Possibly Lost: 712 bytes in 9 blocks
   still reachable: 606,997 bytes in 562 blocks

--- FINAL COUNTS ---
* Total Errors Reported by Valgrind: 21

--- End of Summary ---
```
