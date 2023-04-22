// open_close.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *reccwd; // Pointer for recursive pathname function

extern OFT    oft[NOFT]; // Open File Table          

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** open_close.c file **************/

int dup(int fd)
{
    if (fd < 0 || fd >= NFD){
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    }
    if (running->fd[fd]==0){
        red();
        printf("error : fd not open\n");
        white();
        return -1;
    }
    if (running->fd[fd]->mode){
        red();
        printf("error : fd open in incompatible mode\n");
        white();
        return -1;
    }

    
    for(int i = 0; i < NFD; ++i){
        if (running->fd[i]==0){
            running->fd[i] = running->fd[fd];
            running->fd[fd]->shareCount++;
            return fd;
        }
    }
    red();
    printf("error : no fd avaiable for dup\n");
    white();
    return -1;
}

int dup2(int fd, int gd)
{
    if (fd < 0 || fd >= NFD || gd < 0 || gd >= NFD){
        red();
        printf("error : fd/gd not in range\n");
        white();
        return -1;
    }
    if (running->fd[fd]==0){
        red();
        printf("error : fd not open\n");
        white();
        return -1;
    }
    if (running->fd[fd]->mode){
        red();
        printf("error : fd open in incompatible mode\n");
        white();
        return -1;
    }
    if (running->fd[gd])
        close_file(gd);

    running->fd[gd] = running->fd[fd];
    running->fd[fd]->shareCount++;
    
    return gd;
}

int mylseek(int fd, int position)
{
    int newpos = running->fd[fd]->offset + position;

    if (newpos > running->fd[fd]->inodeptr->INODE.i_size || newpos < 0){
        red();
        printf("error: position out of bounds\n");
        white();
        return -1;
    }
    
    running->fd[fd]->offset += position;
    return position;
}

int pfd()
{
    printf("  fd\t  mode\t  offset\t INODE\n");
    printf(" ----     ----    ------        -------\n");

    for(int i = 0; i < NFD; ++i){
        if (running->fd[i] && running->fd[i]->shareCount > 0){
            printf("  %d\t  ", i);
            switch(running->fd[i]->mode){
                case 0: printf("READ  ");
                        break;
                case 1: printf("WRITE ");
                        break;
                case 2: printf("R & W ");
                        break;
                case 3: printf("APPEND");
                        break;
            }
            printf("%6d\t\t[%d,%4d]\n", running->fd[i]->offset, 
                                        running->fd[i]->inodeptr->dev,
                                        running->fd[i]->inodeptr->ino);
        }
    }
}

int truncate_file(MINODE *mip)
{
    int i, j;
    int ibuf[BLKSIZE], dbuf[BLKSIZE];

    for (int i = 0; i < 12; i++){ // loop through direct blocks of files inode
        if (!mip->INODE.i_block[i]) // if direct block not allocated ( = 0 )
            break;

        bdalloc(dev, mip->INODE.i_block[i]); // deallocate direct block from block 
        mip->INODE.i_block[i] = 0; // set direct block to 0, indicating deallocation
    }

    if (mip->INODE.i_block[12]){ // If there are indirect blocks, print them out
        get_block(dev, mip->INODE.i_block[12], (char *)ibuf);
        i = 0;

        while(ibuf[i] && i < 256){
            bdalloc(dev, ibuf[i]);
            ibuf[i] = 0;
            i++;
        }

        bdalloc(dev, mip->INODE.i_block[12]);
        mip->INODE.i_block[12] = 0;
    }

    if (mip->INODE.i_block[13]){ // If there are double indirect blocks, print them out
        get_block(dev, mip->INODE.i_block[13], (char *)dbuf);
        i = 0;

        while(ibuf[i] && i < 256){
            get_block(dev, dbuf[i], (char *)ibuf);

            j = 0;
            while(ibuf[j] && j < 256){
                bdalloc(dev, ibuf[j]);
                ibuf[j] = 0;
                ++j;
            }

            bdalloc(dev, dbuf[j]);
            dbuf[j] = 0;
            i++;
        }

        bdalloc(dev, mip->INODE.i_block[13]);
        mip->INODE.i_block[13] = 0;
    }

    mip->INODE.i_blocks = 0; // set total number of blocks used by file to 0
    mip->INODE.i_size = 0; // set file size to 0
    mip->modified = 1; // set modified flag to 1
}

int open_file()
{
    if (strlen(pathname)==0){ 
        red();
        printf("error : no filename specified\n");
        white();
        return -1;
    }

    // if (pathname[0] != '/'){ // process for building absolute pathname
    //     reccwd = running->cwd; // saves running->cwd, which will be modified in recAbsPath
    //     recAbsPath(running->cwd);
    //     running->cwd = reccwd;
    //     strcat(absPath, pathname);
    //     strcpy(pathname, absPath);
    // }

    MINODE *mip = path2inode(pathname);
    int mode = atoi(parameter);

    if (!mip){
        if (!mode){
            red();
            printf("error : file does not exist\n");
            white();
            return -1;
        }
        creat_file();
        mip = path2inode(pathname);
        printf("mip dev=%d ino=%d\n", dev, mip->ino);
    }

    if (!S_ISREG(mip->INODE.i_mode)){
        red();
        printf("error : file is not regular file\n");
        white();
        iput(mip);
        return -1;
    }

    for (int i = 0; i < NFD; ++i){
        if (running->fd[i] != 0 && running->fd[i]->inodeptr == mip &&  (running->fd[i]->mode != 0 || mode != 0) ){
            red();
            printf("error : file already opened with incompatible mode\n");
            white();
            iput(mip);
            return -1;
        }
    }


    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = mode;
    oftp->shareCount = 1;
    oftp->inodeptr = mip;

    switch(mode){
        case 0 : oftp->offset = 0;
                 break;
        case 1 : truncate_file(mip);
                 oftp->offset = 0;
                 break;
        case 2 : oftp->offset = mip->INODE.i_size;
                 break;
        case 3 : oftp->offset = mip->INODE.i_size;
                 break;
        default: red();
                 printf("invalid mode\n");
                 white();
                 iput(mip);
                 return -1;
    }

    int i = 0;
    for (; i < NFD; ++i){
        if (running->fd[i] == 0){
            running->fd[i] = oftp;
            break;
        }
    }

    mip->INODE.i_atime = time(0L);
    if (mode)
        mip->INODE.i_mtime = time(0l);
    mip->modified = 1;

    return i;
}

int close_file(int fd)
{
    if (fd < 0 || fd >= NFD){
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    }
    if (running->fd[fd] == 0){
        red();
        printf("error : fd not found\n");
        white();
        return -1;
    }

    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;

    oftp->shareCount--;
    if (oftp->shareCount > 0)
        return 0;

    MINODE *mip = oftp->inodeptr;
    iput(mip);

    free(oftp);
}
