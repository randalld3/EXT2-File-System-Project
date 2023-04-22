// read.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *reccwd; // Pointer for recursive pathname function

extern OFT    oft[NOFT]; // Open File Table          

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** read.c file **************/

int read_file()
{
    char *buf[BLKSIZE];
    if (!pathname[0]){
        red();
        printf("error : fd not specified\n");
        white();
        return -1;
    }
    int fd = atoi(pathname);
    int nbytes = atoi(parameter);
    if(!running->fd[fd]){
        red();
        printf("error : fd not opened\n");
        white();
        return -1;
    }
    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2){
        red();
        printf("error : fd in incompatible mode\n");
        white();
        return -1;
    }

    return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes)
{
    int lbk, blk, startByte, count = 0;
    OFT *oftp = running->fd[fd];
    int avil = oftp->inodeptr->INODE.i_size - oftp->offset;
    char *cq = buf;
    
    int ibuf[BLKSIZE], dbuf[BLKSIZE];
    char readbuf[BLKSIZE];

    if (nbytes > avil)
        nbytes = avil;

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

    // printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

