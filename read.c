// read.c
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

int read_file()
{
    char *buf[BLKSIZE];
    if (!pathname[0]){
        printf("error : fd not specified\n");
        return -1;
    }
    int fd = atoi(pathname);
    int nbytes = atoi(parameter);
    if(!running->fd[fd]){
        printf("error : fd not opened\n");
        return -1;
    }
    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2){
        printf("error : fd in incompatible mode\n");
        return -1;
    }

    return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes)
{
    int lbk, blk, startByte, count = 0;
    int avil = running->fd[fd]->inodeptr->INODE.i_size - running->fd[fd]->offset;
    char *cq = buf;
    OFT *oftp = running->fd[fd];
    int ibuf[BLKSIZE], dbuf[BLKSIZE];
    char readbuf[BLKSIZE];

    while (nbytes && avil){
        /*Compute LOGICAL BLOCK number lbk and startByte in that block from offset*/
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12) // lbk is a direct block
            blk = oftp->inodeptr->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
        else if (lbk >= 12 && lbk < 256 + 12){
            get_block(dev, oftp->inodeptr->INODE.i_block[12], ibuf);
            blk = ibuf[lbk-12];
        }
        else if (lbk >= 256 + 12){
            get_block(dev, oftp->inodeptr->INODE.i_block[13], (char *)ibuf);

            lbk = lbk - (BLKSIZE / sizeof(int)) - 12;
            blk = ibuf[lbk / (BLKSIZE / sizeof(int))];

            get_block(dev, blk, dbuf);

            lbk = lbk % (BLKSIZE / sizeof(int));
            blk = dbuf[lbk];
        }

        /* get the data block into readbuf[BLKSIZE] */
        get_block(oftp->inodeptr->dev, blk, readbuf);
        
        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char *cp = readbuf + startByte;
        int remain = BLKSIZE - startByte; // number of bytes remain in readbuf[]

        int cbytes = nbytes <= remain ? nbytes : remain;

        memcpy(cq, cp, cbytes);
        oftp->offset += cbytes;
        count += cbytes;
        avil -= cbytes; 
        cq += cbytes;
        cp += cbytes;

        if (nbytes <= remain){
            remain -= cbytes;
            nbytes = 0;
        }
        else{
            nbytes -= cbytes;
            remain = 0;
        }
    }

    printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

int mycat(char *filename)
{
    char mybuf[BLKSIZE], dummy = 0;
    int n, i;

    strcpy(pathname, filename);
    strcpy(parameter, "0");
    int fd = open_file();

    if (fd < 0)
        return -1;

    while (n = myread(fd, mybuf, BLKSIZE)){
        mybuf[n] = 0;
        i = 0;

        while (mybuf[i]){
            mybuf[i] == '\n' ? putchar('\n') : putchar(mybuf[i]);
            i++;
        }
    }
    putchar('\n');
    close_file(fd);
    return 0;
}