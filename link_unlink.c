//link_unlink.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *root, *reccwd; // Pointer to root directory inode               

// Variables for file descriptor and command processing
extern int  dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** link_unlink.c file **************/

int truncate(MINODE *mip)
{
    int i, j, dbuf[BLKSIZE], ibuf[BLKSIZE];

    for (int i = 0; i < 12; i++){ // loop through direct blocks of files inode
        if (!mip->INODE.i_block[i]) // if direct block not allocated ( = 0 )
            break;

        bdalloc(dev, mip->INODE.i_block[i]); // deallocate direct block from block 
        mip->INODE.i_block[i] = 0; // set direct block to 0, indicating deallocation
    }

    if (mip->INODE.i_block[12]){ 
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

    if (mip->INODE.i_block[13]){ 
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
    iput(mip); // release MINODE, writing back info
}

int link()
{
    char parent[128], child[128];
    char *parentp, *childp; // create pointers to traverse parent and child names
    MINODE *mip, *parampip; // create MINODE pointers for source and target inodes
    char buf[BLKSIZE];

    if (!pathname[0]){ // first character of pathname is null, no pathname specified
        red();
        printf("error : no pathname specified\n"); // print and return error
        white();
        return -1;
    }
    else if (!parameter[0]){ // first character of parameter is null, no parameter specified
        red();
        printf("error : no parameter specified\n");
        white();
        return -1;
    }

    if (pathname[0] != '/'){ // first character of pathname is not absolute path
        reccwd = running->cwd; // save current working directory in reccwd
        recAbsPath(running->cwd); // call function to recursively get absolute path of cwd
        running->cwd = reccwd; // restore current working directory from reccwd
        strcat(absPath, pathname); // concatenate absolute path and pathname to create new pathname
        strcpy(pathname, absPath); // copy new pathname to pathname
    }

    mip = path2inode(pathname); // get MINODe pointer of source inode using pathname
    if (!mip){ // source inode not found
        red();
        printf("error : inode not found at %s\n", pathname); // print and return error message
        white();
        return -1;
    }
    if (S_ISDIR(mip->INODE.i_mode)){ // source inode is a directory
        red();
        printf("not allowed! inode=%d at %s is dir\n", mip->ino, pathname); // print error message
        white();
        iput(mip); // release source inode
        return -1;
    }

    mip->shareCount -=2; // decrement shareCount by 2
    strcpy(parent, parameter); // copy parameter to parent character array
    strcpy(child, parameter);  // copy parameter to child character array

    parentp = dirname(parent); // get directory name from parent character array
    childp = basename(child); // get basename from child character array

    parampip = path2inode(parameter); // get MINODE pointer of parameter inode
    if (parampip){ // parameter inode already exists
        red();
        printf("error: inode=%d at %s already exits\n", parampip->ino, parameter);
        white();
        iput(parampip); // release parameter inode
        return -1;
    }

    parampip = path2inode(parentp); // get MINODE pointer of parent inode
    if (!S_ISDIR(parampip->INODE.i_mode)){ // source inode is not a directory
        red();
        printf("error : inode=%d at %s is not dir\n", parampip->ino, parentp);
        white();
        iput(parampip); // release parent inode
        return -1;
    }
    
    enter_child(parampip, mip->ino, childp); // add child entry to parent directory
    mip->INODE.i_links_count++; // increment link count of source inode
    mip->modified = 1; // mark source code as modified
    parampip->modified = 1; // mark parent inode as modified
    parampip->shareCount--; // decrement shareCount of parent inode

    iput(mip); // release source inode
    iput(parampip); // release parent inode
}

int unlink()
{
    char child[128], parent[128];
    char *childp, *parentp;
    MINODE *mip, *pip;

    if (!pathname[0]){ // no pathname specified
        red();
        printf("error : no pathname specified\n"); // print and return error message
        white();
        return -1;
    }

    if (pathname[0] != '/'){ // pathanme isn't absolute path
        reccwd = running->cwd; // save cwd
        recAbsPath(running->cwd); // record absolute path of cwd
        running->cwd = reccwd; // restore cwd
        strcat(absPath, pathname); // concat given pathname to absolute path
        strcpy(pathname, absPath); // copy absolute path back to given pathname
    }

    mip = path2inode(pathname); // get MINODE pointer of source inode using given pathname

    if (!mip){ // source inode not found
        red();
        printf("error : inode not found at %s\n", pathname);
        white();
        return -1;
    }
    if (S_ISDIR(mip->INODE.i_mode)){ // source inode is a directory
        red();
        printf("not allowed! inode=%d at %s is dir\n", mip->ino, pathname);
        white();
        iput(mip);  // release source inode
        return -1;
    }

    strcpy(child, pathname); // copy given pathname to child array
    strcpy(parent, pathname); // copy given pathname to parent array
    parentp = dirname(pathname); // get parent directory path from pathname
    childp = basename(child); // get child name from given pathname
    pip = path2inode(parentp); // get MINODE pointer of parent inode using parent directory path
    rm_child(pip, childp); // remove child entry from parent directory inode
    pip->modified = 1; // set parent inode as modified
    iput(pip); // release parent inode
    mip->INODE.i_links_count--; // decrement link count of source inode

    
    
    if(mip->INODE.i_links_count){ // if link count > 0
        mip->modified = 1; // set source inode as modified
    }
    else{ // if link count of source inode is 0
        truncate(mip); // truncate file content of source inode
        idalloc(dev, mip); // deallocate source inode from disk
    }
    iput(mip); // release source inode
}

int symlink()
{
    char parent[128], child[128], temp[128];
    char *parentp, *childp;
    MINODE *mip, *parampip;
    char buf[BLKSIZE];
    int blk;

    if (!pathname[0]){ // no pathname specified, print and return error
        red();
        printf("error : no pathname specified\n");
        white();
        return -1;
    }
    else if (!parameter[0]){ // no parameter specified, print and return error
        red();
        printf("error : no parameter specified\n");
        white();
        return -1;
    }
    strcpy(temp, pathname); // copy pathname to temp 

    if (pathname[0] != '/'){ // no / in front of pathname
        reccwd = running->cwd; // save cwd
        recAbsPath(running->cwd);  // record absolute path of cwd
        running->cwd = reccwd; // restore cwd
        strcat(absPath, pathname); // append pathname to absPath
        strcpy(pathname, absPath); // copy absPath to pathname
    }

    mip = path2inode(pathname); // get MINODe pointer of source inode using absolute pathname
    if (!mip){ // node source inode found, print and return error
        red();
        printf("error : inode not found at %s\n", pathname);
        white();
        return -1;
    }

    strcpy(pathname, parameter); // copy parameter to pathname
    bzero(absPath, sizeof(absPath)); // clear absPath, setting to 0
    mip->shareCount--; // decrement shareCount
    iput(mip); // release MINODE
    if (creat_file() == -1) // call creat_file and check success status
        return -1;
    

    mip = path2inode(pathname); // get MINODE pointer of newly created file using pathname 
    mip->INODE.i_mode = LMODE; // set mode of newly created file to LMODE (symbolic link)

    mip->INODE.i_block[0] = balloc(dev); // allocate block for storing symbolic link data
     
    strcpy(buf, temp); // copy temp buf containing original pathname to buf
    put_block(dev, mip->INODE.i_block[0], buf); // write contents of buf to allocated block

    mip->INODE.i_size = strlen(temp); // set size of symbolic link to length of original pathanme
    mip->modified = 1; // mark MINODE as modified
    mip->shareCount--; // decrement shareCount
    iput(mip); // release MINODE

}

int readlink(char *fname, char linkname[])
{
    char buf[BLKSIZE];
    MINODE *mip;

    mip = path2inode(fname); // get MINODe point of file
    if (!mip){ // MINODE pointer doesnt exist, print and return error
        red();
        printf("error : inode not found at %s\n", pathname);
        white();
        return -1;
    }
    if (!S_ISLNK(mip->INODE.i_mode)){ // not a symbolic link, print and return error
        red();
        printf("error : ino=%d at %s is not link\n", mip->ino, pathname);
        white();
        iput(mip);
        return -1;
    }

    get_block(dev, mip->INODE.i_block[0], buf); // read data block containing symbolic link data into buf
    strcpy(linkname, buf); // copy contents of buf to provided linkname buf
    iput(mip);  // release MINODE. FIXME ADDED
}