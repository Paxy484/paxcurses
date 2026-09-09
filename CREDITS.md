# Acknowledgments

We would like to thank these people for their hard work:

* **`The Curses & Ncurses Architecture Models`**
  * **Architects:** Ken Arnold (UC Berkeley), Zeyd Ben-Halim, Eric S. Raymond, and Thomas E. Dickey.
  * Pioneered the terminal-independent screen optimization design paradigms, window layout metrics, and terminal state controls that define the universal API interface model utilized by this engine.

* **`GNU Make Build Automation Engine`**
  * **Architects:** Richard Stallman, Roland McGrath, and the FSF Contributors.
  * Provides the foundational build automation loop and dependency tracking structures used to parse Makefiles natively on Linux Mint and MSYS2 environments.

* **`BSD Make (bmake / pmake)`**
  * **Architects:** Adam de Boor, Simon Gerraty, and the BSD Developers.
  * Provides the native, lightweight build engine used to parse and process compilation trees across FreeBSD, NetBSD, and OpenBSD systems.

* **`MSYS2 Software Distribution & POSIX Environment Layer`**
  * **Architects:** Alexey Pavlov, Ray Donnelly, and the MSYS2 Team.
  * Provides the foundational POSIX translation layer, software runtime environments, and native build tool bridges that allow a clean Bourne shell configuration script and strict POSIX Makefiles to run natively on Windows, compiling source code directly into standalone Win32 console binaries.

* **`The Virtual Terminal Standard (ANSI X3.64 / ECMA-48)`**
  * **Architects:** American National Standards Institute & European Computer Manufacturers Association.
  * Defined the foundational, device-independent terminal control escape sequences (`\033[%d;%dH` for cursor telemetry and `\033[30;40m` for palette maps) that allow this engine to render text layouts cleanly without external graphical rendering frameworks.

* **`Git Version Control System`**
  * **Architects:** Linus Torvalds and the Git Development Community.
  * Provides the completely decentralized version control layout used by systems programmers to track, branch, and maintain source repositories locally on their machines without corporate gatekeeping.

* **`OpenWatcom C/C++ Compiler & Linker Engine (wcl386 & wlib)`**
  * **Architects:** Watcom International Corporation, Sybase, and the OpenWatcom Community.
  * Provides the specialized 32-bit linear protected-mode compilation framework used to translate source code AS IS straight into flat x86 DOS static libraries (`.lib`) without any optimization bloat.

* **`GCC / GNU Compiler Collection`**
  * **Architects:** Free Software Foundation and the Open Source Community.
  * Provides the foundational POSIX compilation and archiving infrastructure used to build clean static library targets (`.a`) on Linux Mint and modern Unix environments.

* **`Clang / LLVM Compiler Infrastructure`**
  * **Architects:** Chris Lattner, the LLVM Community, and contributors.
  * Provides high-performance, strict standard-compliant code diagnostics and quick binary linking mapping for modern system frameworks.

* **`<stdio.h>, <stdlib.h>, <stdarg.h>, <string.h>`**
  * **Architects:** Dennis Ritchie, Brian Kernighan, and the ISO C Committees.
  * Provides the foundational C runtime primitive structures, unbuffered I/O data streams, variable argument memory processing, and raw string array byte manipulations.

* **`<stdint.h>, <stddef.h>, <stdbool.h>`**
  * **Architects:** ISO/IEC JTC 1/SC 22/WG14 Committee (C99 Specification)
  * Provides explicit, invariant integer bit-widths (`uint32_t`, `uint8_t`), accurate object size matrices (`size_t`), and native boolean definitions to guarantee memory safety during cell packing operations.

* **`<termios.h>, <unistd.h>, <sys/ioctl.h>, <sys/select.h>`**
  * **Architects:** IEEE Computer Society / The Open Group (POSIX Standards)
  * Provides the raw Unix terminal register controllers, I/O geometry polling macros, and non-blocking input multiplexing needed to capture key telemetry instantly on Linux Mint and FreeBSD.

* **`<windows.h>`**
  * **Architects:** Microsoft Corporation (Win32 API Engineers)
  * Provides the Windows virtual terminal processing registers, input matrix trackers, and output console handle states used to manage native terminal rendering inside standard Windows shells.

* **`<conio.h>`**
  * **Architects:** Lattice C, Borland, and OpenWatcom Compiler Architects.
  * Provides the classic, retro console unbuffered keyboard polling routines (`_kbhit()`, `_getch()`) used across old x86 DOS targets and Windows MinGW cross-compilation subsystems.

***

## THANK YOU!

And most importantly, **THANK YOU** to you, the programmer, the tester, the person, any person at all!