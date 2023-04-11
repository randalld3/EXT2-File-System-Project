// cd_ls_pwd.c

#include "type.h"

//*********** globals in main.c **********

// global variables defined in main.c that are used in this file
extern PROC   proc[NPROC];       // array of all PROC structures
extern PROC   *running;          // pointer to the current running PROC structure

extern MINODE minode[NMINODE];   // array of all MINODE structures
extern MINODE *freeList;         // pointer to the head of the free MINODE list
extern MINODE *cacheList;        // pointer to the head of the cached MINODE list
extern MINODE *root, *reccwd;             // pointer to the root MINODE

extern OFT    oft[NOFT];         // array of all OFT structures

extern char gline[256];          // global buffer to hold token strings of a pathname
extern char *name[64];           // array of token string pointers
extern int  n;                   // number of token strings in the pathname

extern int ninodes, nblocks;     // number of inodes and number of blocks in the device
extern int bmap, imap, inodes_start, iblk;  // block numbers for the bitmap, inode table, and the first data block on the device

extern int  fd, dev;             // file descriptor and device number of the current file
extern char cmd[16], pathname[128], parameter[128]; // command, pathname, and parameter strings
extern int  requests, hits;      // number of requests and hits for the cache

int cd()
{
    MINODE* mip = path2inode(pathname); // gets the MINODE of the directory specified in the pathname

    if (mip){
        if ((mip->INODE.i_mode & 0xF000) == 0x4000){ // checks if the MINODE is a directory
            printf("cd to [dev ino]=[%d %d]\n", mip->dev, mip->ino);
            iput(running->cwd); // releases the current working directory MINODE
            running->cwd = mip; // sets the current working directory to the new directory
            printf("after cd : cwd = [%d %d]\n", running->cwd->dev, running->cwd->ino);
            return mip;
        }
    }
    printf("%s not a directory\n", pathname); // if directory not found
    return 0;
}

int ls_file(MINODE *mip, char *fname)
{
    char ftime[64]; // buffer to hold the formatted time string
    int i; 
    char linkname[MAX]; // buffer for symbolic link name
    char *t1 = "xwrxwrxwr-------"; // permission string for owner/group/others with rwx and - flags
    char *t2 = "----------------"; // permission string with all - flags

    // prints the file type based on the mode field of the INODE
    if ((mip->INODE.i_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((mip->INODE.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((mip->INODE.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
        
    // prints the permission string based on the mode field of the INODE
    for (i = 8; i >= 0; i--)
    {
        if (mip->INODE.i_mode & (1 << i))
            printf("%c", t1[i]); // print r|w|x 
        else
            printf("%c", t2[i]); // or print -
    }
    // print link count, gid, uid, size and time
    printf("%4d ", mip->INODE.i_links_count); // link count
    printf("%4d ", mip->INODE.i_gid);   // gid
    printf("%4d ", mip->INODE.i_uid);   // uid
    printf("%8ld ", mip->INODE.i_size);  // file size
    time_t mytime = mip->INODE.i_mtime;
    strcpy(ftime, ctime(&mytime));
    // strcpy(ftime, ctime(&mip->INODE.i_mtime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;        // kill \n at end
    printf("%s ", ftime);                
    printf("%s ", fname); // print file basename
    // print -> linkname of symbolic file
    if ((mip->INODE.i_mode & 0xF000) == 0xA000){ // is linked
        readlink(fname, linkname);  // use readlink() to read linkname
        printf(" -> %s", linkname); // print linked name 
    }

    printf("[%d %d]", mip->dev, mip->ino);
    putchar('\n');
}

int ls_dir(MINODE *pip)
{
    char sbuf[BLKSIZE], name[MAX];
    DIR  *dp;
    char *cp;
    MINODE* mip;

    get_block(dev, pip->INODE.i_block[0], sbuf);    // Read the first block of the directory's data into a buffer
    dp = (DIR *)sbuf;    // dp points to the first directory entry within the buffer
    cp = sbuf;    // cp points to the start of the buffer
    printf("i_block[0] = %d\n", pip->INODE.i_block[0]);

    // Loop through all of the directory entries within the buffer
    while (cp < sbuf + BLKSIZE){

        // Copy the name of the current directory entry into the 'name' buffer and null-terminate it
        strncpy(name, dp->name, dp->name_len);
        name[dp->name_len] = 0;
        mip = iget(dev, dp->inode);   // Get the MINODE structure for the current file
        ls_file(mip, name);   // Call ls_file() to print information about the current file
        iput(mip);   // Release the MINODE structure for the current file

        // Move the cp and dp pointers forward to the next directory entry
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
}

int ls()
{
    MINODE *mip = running->cwd;    // Get the current working directory from the running process.
    
    ls_dir(mip); // Call ls_dir() to print information about the files within the current directory.
    // iput(mip); // FIXME TAKEN AWAY Release the MINODE structure for the current working directory.
    
}

int pwd()
{
    MINODE *wd = running->cwd; // Set a pointer to the current working directory of the running process

    if (wd == root)    // If the working directory is the root directory, print "/"
        printf("/\n");
    else    // Otherwise, recursively print the path to the working directory
        rpwd(wd);

    putchar('\n');     // Print a newline character at the end of the output

}

// Define a helper function called rpwd, which takes a MINODE pointer as an argument
rpwd(MINODE *wd)
{
    MINODE *pip;  // Pointer to parent INODE
    char buf[BLKSIZE], myname[256];   // Local string buffer for the name of each directory
    int ino, parentino; // Inode numbers for the working directory and its parent

    if (wd==root) // If the working directory is the root directory, return
        return;

    // Find the inode number of the parent directory, and get the parent's MINODE
    parentino = findino(wd, &ino);
    pip = iget(dev, parentino);

    get_block(dev, wd->INODE.i_block[0], buf); // Get the block containing the directory entries for the working directory

    findmyname(pip, ino, myname); // Find the name of the working directory within its parent directory

    rpwd(pip);    // Recursively call rpwd on the parent directory
    iput(pip);    // Release the parent MINODE

    printf("/%s", myname);    // Print the name of the working directory, preceded by a slash
}


