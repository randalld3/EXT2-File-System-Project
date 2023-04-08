// type.h
#ifndef TYPE_H
#define TYPE_H
/***** type.h file for CS360 Project *****/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include <ext2fs/ext2_fs.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR; 

#define BLKSIZE           1024   // size of a block in bytes
#define MAX                256   // maximum length of a pathname

#define NPROC                2   // maximum number of running processes
#define NMINODE             64   // maximum number of inodes
#define NFD                  8   // maximum number of open files per process
#define NOFT                32   // maximum number of open files in system
#define DMODE           0x41ED   // constant INODE.s_mode == dir
#define RMODE           0x81A4   // constant INODE.s_mode == reg
#define LMODE           0xA1FF   // constant INODE.s_mode == lnk

// In-memory inodes structure
typedef struct minode{		
    INODE INODE;            // disk INODE
    int   dev, ino;
    int   cacheCount;       // minode in cache count
    int   shareCount;       // number of users on this minode
    int   modified;         // modified while in memory
    int   id;               // index ID
    struct minode *next;    // pointer to next minode

}MINODE;

// Open File Table structure
typedef struct oft{
    int   mode;                  // file open mode (read, write, append, etc.)
    int   shareCount;            // number of processes sharing this OFT
    struct minode *inodeptr;     // pointer to the inode of the opened file
    long  offset;                // current position within the file
} OFT;

// PROC structure
typedef struct proc{
    int   uid;                   // user ID of the process (0 or nonzero)
    int   gid;                   // group ID of the process (same as uid)
    int   pid;                   // process ID (1 or 2)
    struct minode *cwd;          // pointer to the current working directory of the process
    OFT   *fd[NFD];              // array of file descriptors for the process
} PROC;

#endif