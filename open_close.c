// open_close.c
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

int dup(int fd)
{
    if (fd < 0 || fd >= NFD){
        printf("error : fd not in range\n");
        return -1;
    }
    if (running->fd[fd]==0){
        printf("error : fd not open\n");
        return -1;
    }
    if (running->fd[fd]->mode){
        printf("error : fd open in incompatible mode\n");
        return -1;
    }

    
    for(int i = 0; i < NFD; ++i){
        if (running->fd[i]==0){
            running->fd[i] = running->fd[fd];
            running->fd[fd]->shareCount++;
            return fd;
        }
    }
    printf("error : no fd avaiable for dup\n");
    return -1;
}

int dup2(int fd, int gd)
{
    if (fd < 0 || fd >= NFD || gd < 0 || gd >= NFD){
        printf("error : fd/gd not in range\n");
        return -1;
    }
    if (running->fd[fd]==0){
        printf("error : fd not open\n");
        return -1;
    }
    if (running->fd[fd]->mode){
        printf("error : fd open in incompatible mode\n");
        return -1;
    }
    if (running->fd[gd])
        close_file(gd);

    running->fd[gd] = running->fd[fd];
    running->fd[fd]->shareCount++;
    
    return gd;
}

int useek(int fd, int position)
{
    for(int i = 0, temp = 0; i < NFD; ++i){
        if (running->fd[i] == fd){
            temp = running->fd[i]->offset + position;
            if (temp > running->fd[i]->inodeptr->INODE.i_size || temp < 0){
                printf("error: position out of bounds\n");
                return -1;
            }
            temp = running->fd[i]->offset;
            running->fd[i]->offset += position;
            return temp;
        }
    }
    printf("error : fd not found\n");
    return -1;
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
    if (!pathname[0]){ // need dir to remove
        printf("error : no filename specified\n");
        return -1;
    }

    if (pathname[0] != '/'){ // process for building absolute pathname
        reccwd = running->cwd; // saves running->cwd, which will be modified in recAbsPath
        recAbsPath(running->cwd);
        running->cwd = reccwd;
        strcat(absPath, pathname);
        strcpy(pathname, absPath);
    }

    MINODE *mip = path2inode(pathname);
    int mode = atoi(parameter);

    if (!mip){
        if (!mode){
            printf("error : file does not exist\n");
            return -1;
        }
        creat_file();
        mip = path2inode(pathname);
        printf("mip dev=%d ino=%d\n", dev, mip->ino);
    }

    if (!S_ISREG(mip->INODE.i_mode)){
        printf("error : file is not regular file\n");
        iput(mip);
        return -1;
    }

    for (int i = 0; i < NFD; ++i){
        if (running->fd[i] != 0 && running->fd[i]->inodeptr == mip &&  (running->fd[i]->mode != 0 || mode != 0) ){
            printf("error : file already opened with incompatible mode\n");
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
        default: printf("invalid mode\n");
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
        printf("error : fd not in range\n");
        return -1;
    }
    if (running->fd[fd] == 0){
        printf("error : fd not found\n");
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
