//mkdir_creat.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *root, *reccwd; // Pointer to root directory inode               

// Variables for file descriptor and command processing
extern int  dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** mkdir.c file **************/

int make_dir()
{
    char parent[128], child[128];
    char *parentp, *childp;
    MINODE *mip, *pip;
    char buf[BLKSIZE];

    if (!pathname[0]){ // no pathname provided
        red();
        printf("error : no pathname specified\n"); // print and return error
        white();
        return -1;
    }

    if (pathname[0] != '/'){ // no / added on front
        reccwd = running->cwd; // save cwd into reccwd
        recAbsPath(running->cwd); // record absolute path of cwd
        running->cwd = reccwd; // set cwd to saved path
        strcat(absPath, pathname); // concat pathnme to absolute path
        strcpy(pathname, absPath); // copy absolute path to pathname
    }

    strcpy(parent, pathname); // copy pathname to parent
    strcpy(child, pathname); // copy pathname to child

    parentp = dirname(parent); // get directory name from parent and store in parentp
    childp = basename(child); // get file name from child and store in childp

    pip = path2inode(parent); // get MINODE structure of parent directory and store in pip

    if (!pip){ // check if parent inode not found
        red();
        printf("inode at %s not found\n", parent); // print and return error
        white();
        return -1;
    }
    if (!S_ISDIR(pip->INODE.i_mode)){ // check if parent inode is not directory
        red();
        printf("inode %d at %s is not dir\n", pip->ino, parent); // print and return error
        white();
        iput(pip);
        return -1;
    }
    mip = path2inode(pathname); // get MINODe structure of pathname and store in mip
    if (mip){ // check if child inode already exists
        red();
        printf("cannot mkdir : child inode %d already present at %s\n", mip->ino, pathname); // print and return error
        white();
        iput(mip); // release child MINODe
        return -1;
    }

    mymkdir(pip, childp); // create new directory with child name in parent directory
    pip->INODE.i_links_count++; // increment link count of parent inode
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0L); // update access time of parent inode
    pip->modified = 1; // mark parent MINODE as modified
    iput(pip); // release parent MINODE

}

int mymkdir(MINODE *pip, char *name) 
{
    char buf[BLKSIZE];
    int ino = ialloc(dev); // allocate inode for new directory
    int bno = balloc(dev); // allocate block for new directory

    MINODE *mip = iget(dev,ino); // load new inode into memory
    INODE *ip = &mip->INODE; // point to actual inode structure

    ip->i_mode = DMODE;		// OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = BLKSIZE;		// Size in bytes 
    ip->i_links_count = 2;	        // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
    
    for(int i = 1; i < 15; i++)
        ip->i_block[i] = 0; // initialize remaining data block pointers to 0 (unused)
 
    mip->modified = 1;            // mark minode MODIFIED
    iput(mip);                    // write INODE to disk

    get_block(dev, bno, buf); // read newly allocated block into memory 
    DIR *dp = (DIR *)buf; // create directory entry pointer at beginning of block
    char *cp = buf;

    dp->inode = ino; // set inode number of first directory entry to new inode
    dp->rec_len = 12; // set record length of first directory entry to 12 bytes (.)
    dp->name_len = 1; // set name length of first directory entry to 1 char (.)
    dp->name[0] = '.'; // set name of first directory entry to "."
    
    cp += dp->rec_len; // move directory entry pointer to end of first entry
    dp = (DIR *)cp; // point to second directory entry
    dp->inode = pip->ino; // set inode number of second directory entry to parents inode number
    dp->rec_len = BLKSIZE - 12; // set record length of second directory entry to remaining size of the block
    dp->name_len = 2; // set name length of second direcotry entry to 2 characters (..)
    dp->name[0] = '.'; dp->name[1] = '.'; // set name of second directory entry to ..

    put_block(dev, bno, buf); // write updated block to disk
    enter_child(pip, ino, name); // enter new directory as child of parent directory
}

int enter_child(MINODE *pip, int myino, char *myname)
{
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int IDEAL_LEN, REMAIN, bno;
    int NEED_LEN = 4*( (8 + strlen(myname) + 3)/4 ); // a multiple of 4
    
    for(int i = 0; i < 12; i++){ // loop through direct blocks of parent inode

        if (pip->INODE.i_block[i] == 0) // if block is empty, break
            break;

        get_block(pip->dev, pip->INODE.i_block[i], buf); // read block data into buffer
        dp = (DIR *)buf; // set directory entry pointer to start of buf
        cp = buf; // set character pointer to start at buf

        while(cp < buf + BLKSIZE){ // loop through directory entries within the block
            IDEAL_LEN = 4*( (8 + dp->name_len + 3)/4 );     // multiple of 4. calculate ideal length of entry
            REMAIN = dp->rec_len - IDEAL_LEN; // calculate remaining space in the directory entry

            if (REMAIN >= NEED_LEN){      // found enough space for new entry
                dp->rec_len = IDEAL_LEN;  // trim dp's rec_len to its IDEAL_LEN

                cp += dp->rec_len; // move char pointer to end of current directory entry
                dp = (DIR *)cp; // move directory entry pointer to next entry

                dp->inode = myino; // set inode number of new entry
                dp->rec_len = REMAIN; // set rec_len of new entry to remaining space
                dp->name_len = strlen(myname); // set name length of new entry to length of name
                strcpy(dp->name, myname); // copy name of new entry

                put_block(dev, pip->INODE.i_block[i], buf); // write updated block back to disk

                return 1;
            }

            cp += dp->rec_len; // move character pointer to next directory entry
            dp = (DIR *)cp; // move directory entry pointer to next entry
        }
    }

}

int creat_file()
{
    char parent[128], child[128];
    char *parentp, *childp;
    MINODE *mip, *pip;

    if (!pathname[0]){ // check if no pathname is specified
        red();
        printf("error : no pathname specified\n"); // print and return error
        white();
        return -1;
    }

    if (pathname[0] != '/'){ // check if pathname starts with /
        reccwd = running->cwd; // save cwd
        recAbsPath(running->cwd); // get absolute path of cwd
        running->cwd = reccwd; // set cwd to recorded cwd
        strcat(absPath, pathname); // concat pathname onto absolute path
        strcpy(pathname, absPath); // copy absolute path to pathname
    }

    strcpy(parent, pathname); // copy pathname to parent
    strcpy(child, pathname); // copy pathname to child

    parentp = dirname(parent); // get parent directory path using dirname
    childp = basename(child); // get child basename
    printf("pathname=%s\ndirname=%s\nbasename=%s\n", pathname, parentp, childp); // print original pathname, parent directory path, child basename

    pip = path2inode(parent); // get MINODE pointer for parent directory inode
    if (!pip){ // check if parent INODE exists
        red();
        printf("inode at %s not found\n", parentp); // print and return error
        white();
        return -1;
    }
    if (!S_ISDIR(pip->INODE.i_mode)){ // check if parent directory INODE is not a directory
        red();
        printf("inode %d at %s is not dir\n", pip->ino, parentp); // print and return error
        white();
        iput(pip); // release parent MINODE
        return -1;
    }
    mip = path2inode(pathname); // get MINODE pointer for specified pathname 
    if (mip){ // check if specified pathname exists already
        red();
        printf("cannot creat : child inode %d already present at %s\n", mip->ino, pathname); // print and return error
        white();
        iput(mip); // release child INODE
        return -1;
    }

    my_creat(pip, childp); // call my_creat to create file
    pip->INODE.i_atime = time(0L); // update access time of parent directory
    pip->modified = 1; // set parent directory MINODE as modified
    iput(pip); // release parent MINODE

}

int my_creat(MINODE*pip, char *name)
{
    char buf[BLKSIZE];
    int ino = ialloc(dev);
    int bno = balloc(dev);

    MINODE *mip = iget(dev,ino); // get MINODE with newly allocated inode
    INODE *ip = &mip->INODE; // get INODE from MINODE

    ip->i_mode = RMODE;		// OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = 0;     		// Size in bytes 
    ip->i_links_count = 1;	        // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
    
    for(int i = 1; i < 15; i++)
        ip->i_block[i] = 0; // initialize rest of data blocks to 0
 
    mip->modified = 1;            // mark minode MODIFIED
    iput(mip);                    // write INODE to disk
    enter_child(pip, ino, name); // add new entry for file in parent directory with given name and newly allocated inode
}