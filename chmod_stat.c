//chmod_stat.c
#include "type.h"
/*********** globals in main.c ***********/

// Variables for file descriptor and command processing
extern char pathname[128], parameter[128];


int do_chmod()
{
    if (!pathname[0]){ //first character in pathname is null
        red();
        printf("chmod mode filename\n"); 
        white();
        return -1; // return -1 for error
    }
    MINODE *mip = path2inode(parameter); // get MINODE (memory inode) of file specified
    if (!mip){ // MINODE not found
        printf("no such file %s\n", parameter); // no file exists
        return -1; // return -1 for error
    }

    int v = 0;
    char *cp = pathname; // set cp to point to beginninf of pathname string 
    while(*cp){ // while cp exists
        v = 8*v + *cp - '0'; // convert string of characters at cp to integer and store in v
        cp++; // get next character for cp
    }

    v = v & 0x01FF;                         // keep only last 9 bits (bitwise AND)
    printf("mode=%s v=%d\n", pathname, v); // print mode and value of v
    mip->INODE.i_mode &= 0xFE00;            // turn off last 9 bits to 0
    mip->INODE.i_mode |= v;                 // or in v value to set new mode of files inode
    mip->modified = 1; // set modified flag of MINODE to indicate changes have been made
    iput(mip); // release MINODE, writing back to memory
    return 0;
}