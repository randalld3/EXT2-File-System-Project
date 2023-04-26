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
    if (strlen(pathname)==0){ // check if file path specified or not
        red();
        printf("error : file not specified\n");
        white();
        return -1;
    }

    bzero(parameter, sizeof(parameter)); // clear parameter buffer
    int fd = open_file(); // open file and get file descriptor
    if (fd == -1) // error opening file
        return -1;

    char buf[BLKSIZE];
    MINODE *mip = running->fd[fd]->inodeptr; // get minode pointer from fd
    get_block(mip->dev, mip->INODE.i_block[0], buf); // read data from first block of file into buffer
    char *cp = buf; // initialize pointer to buffer

    for(int i = 0; i < 10 && cp < buf + BLKSIZE; ++i) // loop through first 10 lines or until end
        while (*cp++ != '\n')  // move pointer to next newline char
            if (*cp == '\0') break; // if end of buffer, break

    *cp = 0; // set char after last line in buf to null
    printf("%s\n", buf);
    close_file(fd);
}

int mytail()
{
    if (strlen(pathname)==0){
        red();
        printf("error : file not specified\n");
        white();
        return -1;
    }

    bzero(parameter, sizeof(parameter)); // clear parameter buffer
    int fd = open_file(); // open file and get file descriptor
    if (fd == -1) // if fd -1, errored
        return -1;

    MINODE *mip = running->fd[fd]->inodeptr; // get minode pointer from file descriptor
    mip->INODE.i_size < BLKSIZE ? mylseek(fd, 0) :
        mylseek(fd, mip->INODE.i_size - BLKSIZE); // set read offset to beginning of last block if file size larger than a block, otherwise 0

    char buf[BLKSIZE];
    int n = myread(fd, buf, BLKSIZE); // read data from file into buffer and get num of bytes read
    char *cp = &buf[n-1]; // initialize pointer to last byte in buf
    *cp = 0; // set last byte in buf to null terminator

    for(int i = 0; i < 10 && cp >= buf; ++i) // loop through last 10 lines or until beginning of buf is reached
        while(*cp-- != '\n' && cp >= buf); // move pointer to previous newline char

    printf("%s\n", cp+1); // print buffer contents as output, starting from char after lat newline
    close_file(fd);
}