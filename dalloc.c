// dalloc.c
#include "type.h"
/*********** globals in main.c ***********/

extern MINODE minode[NMINODE];   // In-memory Inodes structure array

// Variables for holding file system metadata
extern int ninodes, nblocks;
extern int bmap, imap;  // bitmap, inodes block numbers

// Variables for file descriptor and command processing
extern int  dev;

/**************** dalloc.c file **************/

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

