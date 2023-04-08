// alloc.c
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


int ialloc(int dev)  // allocate an inode number from imap block
{
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i)==0){
            set_bit(buf,i);
            put_block(dev, imap, buf);
        
            decFreeInodes(dev);

            printf("ialloc : ino=%d\n", i+1);		
            return i+1;
        }
    }
    return 0;
}

int balloc(int dev)  // allocate an inode number from imap block
{
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++){
        if (tst_bit(buf, i)==0){
            set_bit(buf,i);
            put_block(dev, bmap, buf);
        
            decFreeBlocks(dev);

            printf("balloc : blk=%d\n", i+1);		
            return i+1;
        }
    }
    return 0;
}

MINODE *mialloc() // allocate a FREE minode for use
{
    int i;
    for (i=0; i<NMINODE; i++){ // iterate over all minodes
        MINODE *mp = &minode[i];
        if (mp->shareCount == 0){ // if shareCount is 0, the minode is free
            mp->shareCount = 1; // set shareCount to 1 and return the minode
            return mp;
        }
    }
    printf("FS panic: out of minodes\n"); // if no free minodes, print an error message and return 0
    return 0;
}

int decFreeInodes(int dev){
    char buf[BLKSIZE];

    // dec free inodes count by 1 in SUPER and GD
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int decFreeBlocks(int dev){
    char buf[BLKSIZE];

    // dec free blocks count by 1 in SUPER and GD
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}