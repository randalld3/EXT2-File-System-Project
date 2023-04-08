// dalloc.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   proc[NPROC];   // process table
extern PROC   *running;     // pointer to the currently running process

extern MINODE minode[NMINODE];   // In-memory Inodes structure array
extern MINODE *freeList;         // List of free inodes
extern MINODE *cacheList;        // List of inodes that are currently in use

extern MINODE *root; // Pointer to root directory inode

extern OFT    oft[NOFT]; // Open File Table

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

// Variables for holding file system metadata
extern int ninodes, nblocks;
extern int bmap, imap, inode_start, iblk;  // bitmap, inodes block numbers

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits; // Variables for caching information



int idalloc(int dev, int ino)  // deallocate an ino number
{
    int i;  
    char buf[BLKSIZE];

    // return 0 if ino < 0 OR > ninodes
    if (ino < 0 || ino > ninodes)
        return 0;

    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);

    // write buf back
    put_block(dev, imap, buf);

    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}

int bdalloc(int dev, int blk) // deallocate a blk number
{
    int i;  
    char buf[BLKSIZE];

    // return 0 if ino < 0 OR > nblocks
    if (blk < 0 || blk > nblocks)
        return 0;

    // get block bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, blk-1);

    // write buf back
    put_block(dev, bmap, buf);

    // update free block count in SUPER and GD
    incFreeBlocks(dev);
}

int midalloc(MINODE *mip) // release a used minode
{
    mip->shareCount = 0;  // set shareCount to 0 to release the minode
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    // inc free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

