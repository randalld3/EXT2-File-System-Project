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
    int fd = atoi(pathname);
    printf("ENTERING WRITE\n");
    if (fd < 0 || fd >= NFD){
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    } 
    if (!running->fd[fd] || !running->fd[fd]->mode){
        red();
        printf("error : fd not opened with compatible mode\n");
        white();
        return -1;
    }

    char buf[BLKSIZE];
    int nbytes = strlen(parameter);
    if (nbytes > BLKSIZE){
        red();
        printf("error : write size too large\n");
        white();
        return -1;
    }
    strcpy(buf, parameter);

    return mywrite(fd, buf, nbytes);
}

int mywrite(int fd, char buf[], int nbytes)
{
    char wbuf[BLKSIZE];
    char *cp, *cq = buf;
    int  ibuf[MAX], dbuf[MAX];
    int lbk, blk, tmpblk, startByte, remain, cbytes, tbytes;
    OFT* oftp = running->fd[fd];
    MINODE *mip = oftp->inodeptr;

    while (nbytes > 0){
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12){                         // direct block
            if (mip->INODE.i_block[lbk] == 0)   // if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            
            blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }
        else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks 
          // HELP INFO:
            if (mip->INODE.i_block[12] == 0){
                mip->INODE.i_block[12] = balloc(mip->dev);
                get_block(mip->dev, mip->INODE.i_block[12], dbuf);
                bzero(dbuf, BLKSIZE);
                put_block(mip->dev, mip->INODE.i_block[12], dbuf);
                mip->INODE.i_blocks++;
            }
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
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
            
            if (tmpblk == 0){
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

        bzero(wbuf, BLKSIZE);
        get_block(mip->dev, blk, wbuf);
        cp = wbuf + startByte;
        remain = BLKSIZE - startByte;
        cbytes = nbytes <= remain ? nbytes : remain;

        memcpy(cp, cq, cbytes);
        oftp->offset += cbytes;
        nbytes -= cbytes;
        cq += cbytes;
        tbytes += cbytes;

        if (oftp->offset > mip->INODE.i_size)
            mip->INODE.i_size = oftp->offset;

        put_block(mip->dev, blk, wbuf);
        wbuf[BLKSIZE-1] = '\0';
    }

    mip->modified = 1;
    printf("wrote %d char into file descriptor fd=%d\n", tbytes, fd);           
    return tbytes;
}