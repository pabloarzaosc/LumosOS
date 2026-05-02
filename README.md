# LumosOS

LumosOS is a small experimental operating system written from scratch in C and x86 Assembly.  
It runs in a virtualized environment and is designed for learning low-level systems, operating system concepts, and hardware interaction.

## Features

- Custom bootloader (x86 Assembly)
- Kernel written in freestanding C (no standard library)
- VGA text-mode terminal interface
- Keyboard input handling (scancode-based)
- Command-line shell
- Persistent virtual disk support (raw disk image)
- Custom filesystem with file and directory support

## Command Overview

### System
- `help` – list all commands
- `clear` – clear the screen
- `about` – system information
- `version` – show OS version
- `reboot` – restart the system
- `uptime` – show system runtime

### Navigation
- `ls` – list files and directories
- `cd <dir>` – change directory
- `pwd` – show current path

### Files
- `touch <file>` – create file
- `cat <file>` – read file contents
- `write <file> <text>` – overwrite file
- `append <file> <text>` – append to file
- `rm <file>` – delete file
- `mv <src> <dst>` – move/rename file
- `cp <src> <dst>` – copy file
- `size <file>` – show file size
- `find <name>` – search file

### Directories
- `mkdir <dir>` – create directory
- `rmdir <dir>` – remove directory

### Debug / System Info
- `meminfo` – memory information
- `diskinfo` – disk statistics
- `sysinfo` – system summary

## Example

```
LumosOS:/ > mkdir test
LumosOS:/ > cd test
LumosOS:/test > write file hello
LumosOS:/test > cat file
hello
```

## Project Structure

```
boot/
    boot.asm

kernel/
    kernel.c

filesystem/
    filesystem.c
    filesystem.h

drivers/
    disk.c
    disk.h

shell/
    shell.c
    shell.h
```

## Build

Requirements:
- NASM
- GCC (freestanding)
- Make

Build the project:

```
make
```

## Run

Using QEMU:

```
qemu-system-x86_64 -hda lumos.img -m 512M
```

## Disk

The system uses a virtual disk image:

```
lumos.img
```

This file acts as the OS storage and persists data between runs.
