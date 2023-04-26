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
    if (fd < 0 || fd >= NFD){ // check if fd is out of range
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    }

    if (running->fd[fd]==0){ // check if fd is not open
        red();
        printf("error : fd not open\n");
        white();
        return -1;
    }

    if (running->fd[fd]->mode){ // check if fd is in correct compatability mode
        red();
        printf("error : fd open in incompatible mode\n");
        white();
        return -1;
    }

    for(int i = 0; i < NFD; ++i){ // loop through running process fd array to find available slot for duplicating fd
        if (running->fd[i]==0){ // check if current slot is empty
            running->fd[i] = running->fd[fd]; // duplicate fd by copying file table entry to new slot
            running->fd[fd]->shareCount++; // increment share count of original fd
            return fd; // return duplicated id
        }
    }

    red();
    printf("error : no fd avaiable for dup\n"); // all slots are filled
    white();
    return -1;
}

int dup2(int fd, int gd)
{
    if (fd < 0 || fd >= NFD || gd < 0 || gd >= NFD){ // check if fd, gd out of range
        red();
        printf("error : fd/gd not in range\n"); // fd or gd not in range
        white();
        return -1;
    }

    if (running->fd[fd]==0){ // check if fd is not open
        red();
        printf("error : fd not open\n");
        white();
        return -1;
    }

    if (running->fd[fd]->mode){ // check if fd is open in compatible mode
        red();
        printf("error : fd open in incompatible mode\n"); 
        white();
        return -1;
    }

    if (running->fd[gd]) // check if gd already open, if it is then close
        close_file(gd);

    running->fd[gd] = running->fd[fd]; // duplicate fd by copying its file table entry to gd
    running->fd[fd]->shareCount++; // increment share count of original fd
    return gd;
}

int mylseek(int fd, int position)
{
    int newpos = running->fd[fd]->offset + position; // calculate new position based on current offset and pos

    if (newpos > running->fd[fd]->inodeptr->INODE.i_size || newpos < 0){ // check if new pos is out of bounds
        red();
        printf("error: position out of bounds\n");
        white();
        return -1;
    }

    running->fd[fd]->offset += position; // update offset of fd with given position
    return position; // return given position as result of lseek operation
}

int pfd()
{
    printf("  fd\t  mode\t  offset\t INODE\n");
    printf(" ----     ----    ------        -------\n");

    for(int i = 0; i < NFD; ++i){
        if (running->fd[i] && running->fd[i]->shareCount > 0){ // check if fd is valid and has a positive share count
            printf("  %d\t  ", i); // print fd index

            switch(running->fd[i]->mode){ // print mode of fd
                case 0: printf("READ  ");
                        break;
                case 1: printf("WRITE ");
                        break;
                case 2: printf("R & W ");
                        break;
                case 3: printf("APPEND");
                        break;
            }
            printf("%6d\t\t[%d,%4d]\n", running->fd[i]->offset, // print offset of fd
                                        running->fd[i]->inodeptr->dev, // print device number of corresponding inode
                                        running->fd[i]->inodeptr->ino); // print inode number of inode
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
        get_block(dev, mip->INODE.i_block[12], (char *)ibuf); // read indirect block into ibuf
        i = 0;

        while(ibuf[i] && i < 256){ // loop through ibuf until 0 or reaching end
            bdalloc(dev, ibuf[i]); // deallocate block pointed to by ibuf
            ibuf[i] = 0; // set ibuf at i to 0, showing dealloc
            i++;
        }

        bdalloc(dev, mip->INODE.i_block[12]); // deallocate indirect block
        mip->INODE.i_block[12] = 0; // set indirect block to 0, showing dealloc
    }

    if (mip->INODE.i_block[13]){ // If there are double indirect blocks, print them out
        get_block(dev, mip->INODE.i_block[13], (char *)dbuf); // read double indirect block into dbuf
        i = 0;

        while(ibuf[i] && i < 256){ 
            get_block(dev, dbuf[i], (char *)ibuf); // read block pointed to by dbuf into ibuf array
            j = 0;

            while(ibuf[j] && j < 256){ // through through ibuff until 0 or end
                bdalloc(dev, ibuf[j]); // deallocate block
                ibuf[j] = 0; // 0 indicates deallocation
                ++j;
            }

            bdalloc(dev, dbuf[j]);  // deallocate block
            dbuf[j] = 0; // 0 indicates deallocation
            i++;
        }

        bdalloc(dev, mip->INODE.i_block[13]); // deallocate block
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

    MINODE *mip = path2inode(pathname); // get minode pointer for pathname
    int mode = atoi(parameter); // convert mode parameter to int

    if (!mip){ // check if file exists
        if (!mode){
            red();
            printf("error : file does not exist\n");
            white();
            return -1;
        }

        creat_file(); // creat file if doesnt exist
        mip = path2inode(pathname); // get minode pointer for created file
        printf("mip dev=%d ino=%d\n", dev, mip->ino);
    }

    if (!S_ISREG(mip->INODE.i_mode)){ // check if is regular file
        red();
        printf("error : file is not regular file\n");
        white();
        iput(mip);
        return -1;
    }

    for (int i = 0; i < NFD; ++i){ // loop through all file descriptors
	    // check for file mode compatability
        if (running->fd[i] != 0 && running->fd[i]->inodeptr == mip &&  (running->fd[i]->mode != 0 || mode != 0) ){
            red();
            printf("error : file already opened with incompatible mode\n");
            white();
            iput(mip);
            return -1;
        }
    }

    OFT *oftp = (OFT *)malloc(sizeof(OFT)); // allocate memory for oft entry
    oftp->mode = mode;
    oftp->shareCount = 1;
    oftp->inodeptr = mip;

    switch(mode){
        case 0 : oftp->offset = 0; // read mode, offset = 0
                 break;
        case 1 : truncate_file(mip); // write mode, truncate file
                 oftp->offset = 0;
                 break;
        case 2 : oftp->offset = mip->INODE.i_size; // read/write mod, set offset to file size 
                 break;
        case 3 : oftp->offset = mip->INODE.i_size; // append mode, set offset to file size
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
            running->fd[i] = oftp; // assign oft entry first available fd
            break;
        }
    }

    mip->INODE.i_atime = time(0L); // update access time of file
    if (mode)
        mip->INODE.i_mtime = time(0l); // update modified time of file if in write mode

    mip->modified = 1;
    return i; // return file descriptor index
}

int close_file(int fd)
{
    if (fd < 0 || fd >= NFD){ // check if file descriptor is in range
        red();
        printf("error : fd not in range\n");
        white();
        return -1;
    }

    if (running->fd[fd] == 0){ // check if fd is open
        red();
        printf("error : fd not found\n");
        white();
        return -1;
    }

    OFT *oftp = running->fd[fd]; // get oft pointer for given fd
    running->fd[fd] = 0; // clear fd in running process

    oftp->shareCount--; // decrement share count for oft
    if (oftp->shareCount > 0) // if theres still other procs sharing file, return
        return 0;

    MINODE *mip = oftp->inodeptr; // otherwise, release files inode and free oft mem
    iput(mip);
    free(oftp);
}