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
    int n, i;

    strcpy(pathname, filename);
    strcpy(parameter, "0");
    int fd = open_file();

    if (fd < 0 || fd >= NFD)
        return -1;

    while (n = myread(fd, mybuf, BLKSIZE)){
        mybuf[n] = 0;
        i = 0;

        while (mybuf[i]){
            mybuf[i] == '\n' ? putchar('\n') : putchar(mybuf[i]);
            i++;
        }
    }
    putchar('\n');
    close_file(fd);
    return 0;
}

int mycp()
{
    if (strlen(pathname)==0){
        red();
        printf("error : src not specified\n");
        white();
        return -1;
    }
    if (strlen(parameter)==0){
        red();
        printf("error : dst not specified\n");
        white();
        return -1;
    }

    char dst[128];
    strcpy(dst, parameter);
    bzero(parameter, sizeof(parameter));

    int fd = open_file();
    if (fd == -1){
        red();
        printf("error : cannot complete cp\n");
        white();
        return -1;
    }
    strcpy(pathname, dst);
    strcpy(parameter, "1");
    printf("pathname=%s\n", pathname);
    printf("parameter=%s\n", parameter);
    int gd = open_file();
      printf("CP3\n");
    if (gd == -1){
        red();
        printf("error : cannot complete cp\n");
        white();
        return -1;
    }

    int n;
    char buf[BLKSIZE];
    
    while (n=myread(fd, buf, BLKSIZE))
        mywrite(gd, buf, n);
}

int mymv()
{
    if (strlen(pathname)==0){
        red();
        printf("error : src not specified\n");
        white();
        return -1;
    }
    if (strlen(parameter)==0){
        red();
        printf("error : dst not specified\n");
        white();
        return -1;
    }

    MINODE *mip = path2inode(pathname);
    if (!mip){
        red();
        printf("error : src does not exist\n");
        white();
        return -1;
    }

    if (running->cwd->dev == mip->dev){
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        link();
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        unlink();
    }
    else{
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        mycp();
        printf("pathname=%s parameter=%s\n", pathname, parameter);
        unlink();
    }


}