// write.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process
extern MINODE *reccwd; // Pointer for recursive pathname function
extern OFT    oft[NOFT]; // Open File Table          

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** write.c file **************/


int write_file()
{
    int fd = atoi(pathname); // convert pathname to int to get fd

    if (fd < 0 || fd >= NFD){ // check if fd is within valid range
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    } 

    if (!running->fd[fd] || !running->fd[fd]->mode){ // check if file descriptor is valid and opened with compatible mode
        red();
        printf("error : fd not opened with compatible mode\n");
        white();
        return -1;
    }

    char buf[BLKSIZE];
    int nbytes = strlen(parameter); // get number of bytes to be written from parameter

    if (nbytes > BLKSIZE){ // check if bytes to be written is larger than buff size
        red();
        printf("error : write size too large\n");
        white();
        return -1;
    }

    strcpy(buf, parameter); // copy data from parameter to buffer
    return mywrite(fd, buf, nbytes);
}

int mywrite(int fd, char buf[], int nbytes)
{
    char wbuf[BLKSIZE];
    char *cp, *cq = buf;
    int  ibuf[MAX], dbuf[MAX];
    int lbk, blk, tmpblk, startByte, remain, cbytes, tbytes=0;
    OFT* oftp = running->fd[fd]; // get fd from running proc
    MINODE *mip = oftp->inodeptr; // get inode pointer from fd

    while (nbytes > 0){ 
        lbk = oftp->offset / BLKSIZE; // calculate logical block number based on offset
        startByte = oftp->offset % BLKSIZE; // calculate starting byte within block

        if (lbk < 12){                         // direct block
            if (mip->INODE.i_block[lbk] == 0)   // if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block

            blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }

        else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks 
            if (mip->INODE.i_block[12] == 0){ // if indirect block has not yet been allocated
                mip->INODE.i_block[12] = balloc(mip->dev); // allocate block

                get_block(mip->dev, mip->INODE.i_block[12], dbuf); // read block from indirect block
                bzero(dbuf, BLKSIZE); // zero out parameters
                put_block(mip->dev, mip->INODE.i_block[12], dbuf); // write block back

                mip->INODE.i_blocks++; // increment amount of blocks
            }

            get_block(mip->dev, mip->INODE.i_block[12], ibuf); // read block in
            blk = ibuf[lbk-12]; 

            if (blk == 0){ 
                blk = balloc(mip->dev);
                ibuf[lbk-12] = blk;
                mip->INODE.i_blocks++;
            }

            put_block(mip->dev, mip->INODE.i_block[12], ibuf);
        }
        else{
            lbk -= (12 + MAX);

            if (mip->INODE.i_block[13] == 0){
                mip->INODE.i_block[13] = balloc(mip->dev);

                get_block(mip->dev, mip->INODE.i_block[13], dbuf);
                bzero(dbuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[13], dbuf);

                mip->INODE.i_blocks++;
            }
            bzero(dbuf, BLKSIZE);
            bzero(ibuf, BLKSIZE);

            get_block(mip->dev, mip->INODE.i_block[13], dbuf);
            tmpblk = dbuf[lbk / MAX];

            if (tmpblk == 0){ // if first level indirect block not yet allocated, allocate block and update double indirect
                tmpblk = balloc(mip->dev);
                dbuf[lbk / MAX] = tmpblk;

                get_block(mip->dev, tmpblk, ibuf);
                bzero(ibuf, BLKSIZE);
                put_block(mip->dev, tmpblk, ibuf);

                mip->INODE.i_blocks++;
                put_block(mip->dev, mip->INODE.i_block[13], dbuf);
            }

            bzero(dbuf, BLKSIZE);
            bzero(ibuf, BLKSIZE);
            get_block(mip->dev, tmpblk, dbuf);

            if (dbuf[lbk % MAX] == 0){
                blk = balloc(mip->dev);
                dbuf[lbk % MAX] = blk;
                mip->INODE.i_blocks++;
                put_block(mip->dev, tmpblk, dbuf);
            }
            else
                blk = dbuf[lbk % MAX];
        }

        bzero(wbuf, BLKSIZE); // zero out write buf
        get_block(mip->dev, blk, wbuf); // read block containing data to be written into write buffer

        cp = wbuf + startByte; // set cp to appropriate position in write buffer
        remain = BLKSIZE - startByte; // calculate remaining bytes that can be written in current block
        cbytes = nbytes <= remain ? nbytes : remain; // determine number of bytes to be written now

        memcpy(cp, cq, cbytes); // copy data from cq (source buf) to cp (dest buf) with num of bytes (cbytes)

        oftp->offset += cbytes; // update offset in oft by adding number of bytes written
        nbytes -= cbytes; // update remaining bytes to be written
        cq += cbytes; // move cq by num of bytes to be written
        tbytes += cbytes; // update total num of bytes written by adding num of bytes written

        if (oftp->offset > mip->INODE.i_size) // if offset in oft is greater than size of inodes data (isize), update isize with new offset
            mip->INODE.i_size = oftp->offset;

        put_block(mip->dev, blk, wbuf); // write updated data from write buf back to block on disk
        // wbuf[BLKSIZE-1] = '\0'; // null terminating
    }

    mip->modified = 1; // mark mip as modified
    printf("wrote %d char into file descriptor fd=%d\n", tbytes, fd); // print num of chars written and where          
    return tbytes; // return total number of bytes written
}