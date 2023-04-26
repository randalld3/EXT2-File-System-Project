// cat_cp.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process
extern MINODE *reccwd; // Pointer for recursive pathname function
extern OFT    oft[NOFT]; // Open File Table          

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** cat_cp.c file **************/


int mycat(char *filename)
{
    char mybuf[BLKSIZE], dummy = 0;
    int n, i; // loop control variables

    strcpy(pathname, filename); // copy provided filename to global variable pathname
    strcpy(parameter, "0"); // set param to 0 as a flag
    int fd = open_file(); // open file, get fd

    if (fd < 0 || fd >= NFD) // check if fd is valid
        return -1; // if not, exit

    while (n = myread(fd, mybuf, BLKSIZE)){ // read data from file into buffer and get num of bytes
        mybuf[n] = 0; // set last byte in buffer to null term
        i = 0;

        while (mybuf[i]){ // loop through buf contents
            mybuf[i] == '\n' ? putchar('\n') : putchar(mybuf[i]); // print char, or newline if \n found
            i++; // move to next char in buf
        }
    }

    putchar('\n'); // print new file at end
    close_file(fd); // close file
    return 0;
}

int mycp()
{
    if (strlen(pathname)==0){ // check if source file path specified
        red();
        printf("error : src not specified\n");
        white();
        return -1;
    }

    if (strlen(parameter)==0){ // check if destination file path not specified
        red();
        printf("error : dst not specified\n");
        white();
        return -1;
    }

    char dst[128];
    strcpy(dst, parameter); // copy dest file from parameter into dst buffer
    bzero(parameter, sizeof(parameter)); // clear parameter buffer

    int fd = open_file(); // open source file and get file descriptor
    if (fd == -1){
        red();
        printf("error : cannot complete cp\n");
        white();
        return -1;
    }

    strcpy(pathname, dst); // copy destination file path from dst to pathname
    strcpy(parameter, "1"); // set parameter to 1 as flag

    int gd = open_file(); // open destination file and get file descriptor
    if (gd == -1){ // file open error
        red();
        printf("error : cannot complete cp\n");
        white();
        return -1;
    }

    int n;
    char buf[BLKSIZE];

    while (n=myread(fd, buf, BLKSIZE)) // read data from source file into buffer and get num of bytes read
        mywrite(gd, buf, n); // write data from buffer to destination file

    close_file(fd);
    close_file(gd);
    return 0; // return success
}

int mymv()
{
    if (strlen(pathname)==0){ // check if source path is specified
        red();
        printf("error : src not specified\n");
        white();
        return -1;
    }

    if (strlen(parameter)==0){ // check if destination path is specified
        red();
        printf("error : dst not specified\n");
        white();
        return -1;
    }

    MINODE *mip = path2inode(pathname); // get minode pointer for pathname
    if (!mip){ // source file doesnt exist
        red();
        printf("error : src does not exist\n");
        white();
        return -1;
    }

    if (running->cwd->dev == mip->dev){ // check if source and dest are on same device
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        link(); // create link from source to destination
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        unlink(); // unlink source file and directory
    }
    else{
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        mycp(); // copy source to destination
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        unlink(); // unlink source file and directory
    }
    return 0; // return success
}