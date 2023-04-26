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

    if (!pathname[0]){ // Check if file descriptor is specified
        red();
        printf("error : fd not specified\n");
        white();
        return -1;
    }

    int fd = atoi(pathname); // convert file descriptor from string to int
    int nbytes = atoi(parameter); // convert number of bytes to read from string to int

    if(!running->fd[fd]){ // check if file descriptor has opened
        red();
        printf("error : fd not opened\n");
        white();
        return -1;
    }

    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2){ // check if file descriptor is in a compatible mode for reading
        red();
        printf("error : fd in incompatible mode\n");
        white();
        return -1;
    }

    return myread(fd, buf, nbytes); // call myread function 
}

int myread(int fd, char *buf, int nbytes)
{
    int lbk, blk, startByte, count = 0; // logical block number, physical block num, start byte in block, count of bytes read
    OFT *oftp = running->fd[fd]; // get open file table entry for fd
    int avil = oftp->inodeptr->INODE.i_size - oftp->offset; // calculate available bytes to read from current offset
    char *cq = buf; // set current buffer pointer to beginning of input buffer
    int ibuf[BLKSIZE], dbuf[BLKSIZE]; 
    char readbuf[BLKSIZE];

    if (nbytes > avil)
        nbytes = avil; // limit number of bytes to read to available bytes of file

    while (nbytes && avil){
        /*Compute LOGICAL BLOCK number lbk and startByte in that block from offset*/
        lbk = oftp->offset / BLKSIZE; // get lblk from offset
        startByte = oftp->offset % BLKSIZE; // get start byte within block from offset

        if (lbk < 12) // lbk is a direct block
            blk = oftp->inodeptr->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
        else if (lbk >= 12 && lbk < 256 + 12){ //lblk is indirect block
            get_block(dev, oftp->inodeptr->INODE.i_block[12], ibuf); // read indirect block into buf
            blk = ibuf[lbk-12]; // map logical blk to physical block
        }
        else if (lbk >= 256 + 12){ // lblk is double indirect block
            get_block(dev, oftp->inodeptr->INODE.i_block[13], (char *)ibuf); // read double indirect block into ibuf

            lbk = lbk - (BLKSIZE / sizeof(int)) - 12; // get adjusted logical block number within double indirect block
            blk = ibuf[lbk / (BLKSIZE / sizeof(int))]; // map logical block to physical block within double indirect block

            get_block(dev, blk, dbuf); // read indirect block pointed to by double indirect block into dbuf

            lbk = lbk % (BLKSIZE / sizeof(int)); // get lblk within double indirect
            blk = dbuf[lbk]; // map lblk to physical blk
        }

        /* get the data block into readbuf[BLKSIZE] */
        get_block(oftp->inodeptr->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char *cp = readbuf + startByte; // set current pointer in readbuf to start byte
        int remain = BLKSIZE - startByte; // number of bytes remain in readbuf[] 
        int cbytes = nbytes <= remain ? nbytes : remain; // determine number of bytes to copy from current block, take remaining into account
        
        memcpy(cq, cp, cbytes); // copy bytes from cp to cq

        oftp->offset += cbytes; // update file offset in oft
        count += cbytes; // update total number of bytes read
        avil -= cbytes; // update number of bytes available
        cq += cbytes; // move cq to next empty location in buf
        cp += cbytes; // move cp to next byte in current block

        if (nbytes <= remain){ // if nbytes <= remain, all bytes have been copied
            remain -= cbytes; // update remaining bytes in block
            nbytes = 0; // set nbytes to 0 as all bytes have been copied
        }
        else{
            nbytes -= cbytes; // update nbytes to indicate remaining bytes to be copied from subsequent blocks
            remain = 0; // set remain to 0 as all remaining bytes in block have been copied
        }
    }

    return count;   // count is the actual number of bytes read
}