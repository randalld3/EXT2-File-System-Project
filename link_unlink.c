//link_unlink.c
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

int truncate(MINODE *mip)
{
    for (int i = 0; i < 12; i++){
        if (!mip->INODE.i_block[i])
            break;

        bdalloc(dev, mip->INODE.i_block[i]);
        mip->INODE.i_block[i] = 0;
    }

    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->modified = 1;
    iput(mip);
}

int link()
{
    char parent[128], child[128];
    char *parentp, *childp;
    MINODE *mip, *parampip;
    char buf[BLKSIZE];

    if (!pathname[0]){
        printf("error : no pathname specified\n");
        return -1;
    }
    else if (!parameter[0]){
        printf("error : no parameter specified\n");
        return -1;
    }

    if (pathname[0] != '/'){
        reccwd = running->cwd;
        recAbsPath(running->cwd);
        running->cwd = reccwd;
        strcat(absPath, pathname);
        strcpy(pathname, absPath);
    }

    mip = path2inode(pathname);
    if (!mip){
        printf("error : inode not found at %s\n", pathname);
        return -1;
    }
    if (S_ISDIR(mip->INODE.i_mode)){
        printf("not allowed! inode=%d at %s is dir\n", mip->ino, pathname);
        iput(mip);
        return -1;
    }
    mip->shareCount -=2;
    strcpy(parent, parameter);
    strcpy(child, parameter);

    parentp = dirname(parent);
    childp = basename(child);

    parampip = path2inode(parameter);
    if (parampip){
        printf("error: inode=%d at %s already exits\n", parampip->ino, parameter);
        iput(parampip);
        return -1;
    }

    parampip = path2inode(parentp);
    if (!S_ISDIR(parampip->INODE.i_mode)){
        printf("error : inode=%d at %s is not dir\n", parampip->ino, parentp);
        iput(parampip);
        return -1;
    }
    
    enter_child(parampip, mip->ino, childp);
    mip->INODE.i_links_count++;
    mip->modified = 1;
    parampip->modified = 1;
    parampip->shareCount--;

    iput(mip);
    iput(parampip);
}

int unlink()
{
    char child[128], parent[128];
    char *childp, *parentp;
    MINODE *mip, *pip;

    if (!pathname[0]){
        printf("error : no pathname specified\n");
        return -1;
    }

    if (pathname[0] != '/'){
        reccwd = running->cwd;
        recAbsPath(running->cwd);
        running->cwd = reccwd;
        strcat(absPath, pathname);
        strcpy(pathname, absPath);
    }

    mip = path2inode(pathname);

    if (!mip){
        printf("error : inode not found at %s\n", pathname);
        return -1;
    }
    if (S_ISDIR(mip->INODE.i_mode)){
        printf("not allowed! inode=%d at %s is dir\n", mip->ino, pathname);
        iput(mip);
        return -1;
    }

    strcpy(child, pathname);
    strcpy(parent, pathname);
    parentp = dirname(pathname);
    childp = basename(child);
    pip = path2inode(parentp);
    rm_child(pip, childp);
    pip->modified = 1;
    iput(pip);
    mip->INODE.i_links_count--;

    
    
    if(mip->INODE.i_links_count){
        mip->modified = 1;
    }
    else{
        truncate(mip);
        idalloc(dev, mip);
    }
    iput(mip);
}

int symlink()
{
    char parent[128], child[128], temp[128];
    char *parentp, *childp;
    MINODE *mip, *parampip;
    char buf[BLKSIZE];
    int blk;

    if (!pathname[0]){
        printf("error : no pathname specified\n");
        return -1;
    }
    else if (!parameter[0]){
        printf("error : no parameter specified\n");
        return -1;
    }
    strcpy(temp, pathname);

    if (pathname[0] != '/'){
        reccwd = running->cwd;
        recAbsPath(running->cwd);
        running->cwd = reccwd;
        strcat(absPath, pathname);
        strcpy(pathname, absPath);
    }

    mip = path2inode(pathname);
    if (!mip){
        printf("error : inode not found at %s\n", pathname);
        return -1;
    }

    strcpy(pathname, parameter);
    bzero(absPath, sizeof(absPath));
    mip->shareCount--;
    iput(mip);
    if (creat_file() == -1)
        return -1;
    

    mip = path2inode(pathname);
    mip->INODE.i_mode = LMODE;

    mip->INODE.i_block[0] = balloc(dev);
    
    strcpy(buf, temp);
    put_block(dev, mip->INODE.i_block[0], buf);

    mip->INODE.i_size = strlen(temp);
    mip->modified = 1;
    mip->shareCount--;
    iput(mip);

}

int readlink(char *fname, char linkname[])
{
    char buf[BLKSIZE];
    MINODE *mip;

    mip = path2inode(fname);
    if (!mip){
        printf("error : inode not found at %s\n", pathname);
        return -1;
    }
    if (!S_ISLNK(mip->INODE.i_mode)){
        printf("error : ino=%d at %s is not link\n", mip->ino, pathname);
        iput(mip);
        return -1;
    }

    get_block(dev, mip->INODE.i_block[0], buf);
    strcpy(linkname, buf);
    iput(mip); 
}