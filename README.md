# SisOp 2019-1 T2 #
Trabalho 2 de Sistemas Operacionais I (2019/1)

## Overview ##

This is the second and final project of the Operating Systems I course of Computer Science graduation at UFRGS.

The goal is to provide a simple library for dealing with files, in which the user can perform some actions, such as:
1. Create new regular files and directories
2. Create links to files
3. Open, close and delete files
4. Perform write, read, seek and truncate operations on regular files
5. Read directory entries

The proposed file system has the following characteristics:
1. Indexed allocation (inodes style)
2. Bitmaps for space management
3. Directory hierarchy is a DAG
4. Root directory (`/`) has the inode 1 reserved for it
5. Fixed-size directory entries
6. Entries are arranged as a linear list
7. Supports regular files (mandatory)
8. Supports both softlinks and hardlinks
9. Supports both relative and absolute paths

## Instructions ##

To compile the library `libt2fs.a`, enter `make all` in the root directory of the repository.

Alternatively, to compile for x86_64, enter `make all64`. That will use the student-made `bin/apidisk.c` source instead of the professor-provided `lib/apidisk.o`.
There is no guarantee `lib/apidisk.c` has `lib/apidisk.o`'s functionalities fully implemented and 100% correct.

To compile all the programs inside `exemplo/` or `teste/`, you can enter `make all` inside the desired directory.

Alternatively, you can compile the programs of your choice by entering `make this_one`, having a `this_one.c` or `this_one.cpp` file in the directory.

The command `make clean` inside each directory cleans exactly what the command `make all` (plus `make all64` in the root folder) creates.

More information is available (in Portuguese) in the files inside the `material/` folder.

## Authorship ##

Files created (not provided):

1. `src/*`
2. `teste/*`
3. `Makefile`s
4. `lib/apidisk.c`

By:

- Yuri Jaschek <<yuri.jaschek@inf.ufrgs.br>>
- Giovane Fonseca
- Humberto Lentz
- Matheus F. Kovaleski
