# Compilation Guide

This document describes the compilation pipeline and dependency validation steps required to build PaxCurses across distinct operating system matrices.

## COMPILATION DEPENDENCIES
To compile this project, the host operating system must provide:
1. A C compiler (`cc` / `clang` / `gcc` / OpenWatcom `wcl386`)
2. Standard C runtime headers (`glibc`, `musl`, BSD `libc`, `ucrt` in Windows)
3. Standard system build utilities (`make`, `bmake`, or OpenWatcom `wmake`)

---

## DEPENDENCY INSTALLATION (DEBIAN / LINUX MINT / UBUNTU)
Initialize your system's package management engine and install the universal compilation utilities:

```sh
sudo apt update
sudo apt install build-essential clang
```
*Note: `clang` is optional, but available in case you want to use it instead of `gcc`.*

---

## DEPENDENCY INSTALLATION (ARCH LINUX)
Synchronize your system's rolling-release core packages and deploy the native base-devel toolkit:

```sh
sudo pacman -Syu
sudo pacman -S base-devel clang
```
*Note: `clang` is optional, but available in case you want to use it.*

---

## DEPENDENCY INSTALLATION (FREEBSD)
Synchronize the native package management database. The baseline Clang compiler and `bmake` engine are already built natively into the FreeBSD base operating system layer:

```sh
sudo pkg update
```

---

## BUILD INSTRUCTIONS (UNIX-LIKE SYSTEM)
1. Open your terminal environment (`sh` / `bash` / `zsh`) inside the root project directory.
2. Initialize and grant execution permissions to the configuration script:
   ```sh
   chmod +x ./configure
   ```
3. Run the configuration script to probe local system headers, locate the system compiler, and dynamically write a Makefile:
   ```sh
   ./configure
   ```
4. Execute the build automation engine to compile the final static library archive target (`paxcurses.a`):
   ```sh
   make
   ```
5. To wipe compiled object files and restore the source tree to a clean slate (this also self-destructs the generated Makefile):
   ```sh
   make clean
   ```

---

## BUILD INSTRUCTIONS (WINDOWS VIA MSYS2 ENVIRONMENT)
1. Download and initialize the MSYS2 environment subsystem.
2. Select your preferred environment profile and deploy the corresponding package toolchains via the pacman subsystem:

   * **OPTION A: PURE LLVM / CLANG STACK**
     * Launch the "CLANG64" terminal profile environment window.
     * Synchronize and install the LLVM tools:
       ```sh
       pacman -Syu
       pacman -S mingw-w64-clang-x86_64-toolchain
       ```

   * **OPTION B: TRADITIONAL GNU / GCC STACK**
     * Launch the "UCRT64" terminal profile environment window.
     * Synchronize and install the GNU tools:
       ```sh
       pacman -Syu
       pacman -S mingw-w64-x86_64-toolchain
       ```

3. Execute the standard shell commands inside your project path:
   ```sh
   ./configure
   ```
   ```sh
   make
   ```
   The toolchain will compile a native standalone Windows static library archive (`paxcurses.a`).

---

## BUILD INSTRUCTIONS (FREEDOS / DOS ENVIRONMENTS)
Bypass the shell configuration script entirely. Execute your compilation rules natively inside the OpenWatcom toolchain environment using the dedicated `WMAKE.WPJ` file:

```cmd
wmake -f WMAKE.WPJ
```

This compiles the code, outputting a clean 32-bit linear protected-mode DOS static library archive container (`paxcurses.lib`).

To purge the compiled object matrices and libraries under FreeDOS:
```cmd
wmake -f WMAKE.WPJ clean
```