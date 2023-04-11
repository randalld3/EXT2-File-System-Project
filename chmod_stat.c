//chmod_stat.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   proc[NPROC];   // process table
extern PROC   *running;     // pointer to the currently running process

extern MINODE minode[NMINODE];   // In-memory Inodes structure array
extern MINODE *freeList;         // List of free inodes
extern MINODE *cacheList;        // List of inodes that are currently in use

extern MINODE *root, *reccwd; // Pointer to root directory inode

extern OFT    oft[NOFT]; // Open File Table

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

// Variables for holding file system metadata
extern int ninodes, nblocks;
extern int bmap, imap, inode_start, iblk;  // bitmap, inodes block numbers

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128], absPath[128];
extern int  requests, hits; // Variables for caching information

int do_chmod()
{
    if (!pathname[0]){
        printf("chmod mode filename\n");
        return -1;
    }
    MINODE *mip = path2inode(parameter);
    if (!mip){
        printf("no such file %s\n", parameter);
        return -1;
    }

    int v = 0;
    char *cp = pathname;
    while(*cp){
        v = 8*v + *cp - '0';
        cp++;
    }

    v = v & 0x01FF;                         // keep only last 9 bits
    printf("mode=%s v=%d\n", pathname, v);
    mip->INODE.i_mode &= 0xFE00;            // turn off last 9 bits to 0
    mip->INODE.i_mode |= v;                 // or in v value
    mip->modified = 1;
    iput(mip);
    return 0;
}