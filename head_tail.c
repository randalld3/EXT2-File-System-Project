// head_tail.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *reccwd; // Pointer for recursive pathname function

extern OFT    oft[NOFT]; // Open File Table          

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** head_tail.c file **************/

int myhead()
{
    if (strlen(pathname)==0){
        red();
        printf("error : file not specified\n");
        white();
        return -1;
    }

    bzero(parameter, sizeof(parameter));
    int fd = open_file();
    if (fd == -1)
        return -1;
    
    char buf[BLKSIZE];
    MINODE *mip = running->fd[fd]->inodeptr;
    get_block(mip->dev, mip->INODE.i_block[0], buf);
    char *cp = buf;

    for(int i = 0; i < 10 && cp < buf + BLKSIZE; ++i)
        while (*cp++ != '\n') 
            if (*cp == '\0') break;
            
    *cp = 0;
    printf("%s\n", buf);
}

int mytail()
{
    if (strlen(pathname)==0){
        red();
        printf("error : file not specified\n");
        white();
        return -1;
    }

    bzero(parameter, sizeof(parameter));
    int fd = open_file();
    if (fd == -1)
        return -1;

    MINODE *mip = running->fd[fd]->inodeptr;
    mip->INODE.i_size < BLKSIZE ? mylseek(fd, 0) :
        mylseek(fd, mip->INODE.i_size - BLKSIZE);


    char buf[BLKSIZE];
    int n = myread(fd, buf, BLKSIZE);
    char *cp = &buf[n-1];
    *cp = 0;

    for(int i = 0; i < 10 && cp >= buf; ++i)
        while(*cp-- != '\n' && cp >= buf);

    printf("%s\n", cp+1);
}