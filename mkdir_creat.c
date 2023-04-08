//mkdir_creat.c
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

int make_dir()
{
    char parent[128], child[128];
    char *parentp, *childp;
    MINODE *mip, *pip;
    char buf[BLKSIZE];

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

    strcpy(parent, pathname);
    strcpy(child, pathname);

    parentp = dirname(parent);
    childp = basename(child);

    pip = path2inode(parent);
    
    if (!pip){
        printf("inode at %s not found\n", parent);
        return -1;
    }
    if (!S_ISDIR(pip->INODE.i_mode)){
        printf("inode %d at %s is not dir\n", pip->ino, parent);
        return -1;
    }
    mip = path2inode(pathname);
    if (mip){
        printf("cannot mkdir : child inode %d already present at %s\n", mip->ino, pathname);
        return -1;
    }

    mymkdir(pip, childp);
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->modified = 1;
    iput(pip);

}

int mymkdir(MINODE *pip, char *name)
{
    char buf[BLKSIZE];
    int ino = ialloc(dev);
    int bno = balloc(dev);

    MINODE *mip = iget(dev,ino);
    INODE *ip = &mip->INODE;

    ip->i_mode = DMODE;		// OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = BLKSIZE;		// Size in bytes 
    ip->i_links_count = 2;	        // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
    
    for(int i = 1; i < 15; i++)
        ip->i_block[i] = 0;
 
    mip->modified = 1;            // mark minode MODIFIED
    iput(mip);                    // write INODE to disk

    get_block(dev, bno, buf);
    DIR *dp = (DIR *)buf;
    char *cp = buf;

    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    cp += dp->rec_len;
    dp = (DIR *)cp;
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE - 12;
    dp->name_len = 2;
    dp->name[0] = '.'; dp->name[1] = '.';

    put_block(dev, bno, buf);
    enter_child(pip, ino, name);
}

int enter_child(MINODE *pip, int myino, char *myname)
{
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int IDEAL_LEN, REMAIN, bno;
    int NEED_LEN = 4*( (8 + strlen(myname) + 3)/4 ); // a multiple of 4
    
    for(int i = 0; i < 12; i++){

        if (pip->INODE.i_block[i] == 0)
            break;

        get_block(pip->dev, pip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp < buf + BLKSIZE){
            IDEAL_LEN = 4*( (8 + dp->name_len + 3)/4 );     // multiple of 4
            REMAIN = dp->rec_len - IDEAL_LEN;

            if (REMAIN >= NEED_LEN){      // found enough space for new entry
                dp->rec_len = IDEAL_LEN;  // trim dp's rec_len to its IDEAL_LEN

                cp += dp->rec_len;
                dp = (DIR *)cp;

                dp->inode = myino;
                dp->rec_len = REMAIN;
                dp->name_len = strlen(myname);
                strcpy(dp->name, myname);

                put_block(dev, pip->INODE.i_block[i], buf);

                return 1;
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

}

int creat_file()
{
    char parent[128], child[128];
    char *parentp, *childp;
    MINODE *mip, *pip;

    if (!pathname){
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

    strcpy(parent, pathname);
    strcpy(child, pathname);

    parentp = dirname(parent);
    childp = basename(child);
    printf("pathname=%s\ndirname=%s\nbasename=%s\n", pathname, parentp, childp);

    pip = path2inode(parent);
    if (!pip){
        printf("inode at %s not found\n", parentp);
        return -1;
    }
    if (!S_ISDIR(pip->INODE.i_mode)){
        printf("inode %d at %s is not dir\n", pip->ino, parentp);
        return -1;
    }
    mip = path2inode(pathname);
    if (mip){
        printf("cannot creat : child inode %d already present at %s\n", mip->ino, pathname);
        return -1;
    }

    my_creat(pip, childp);
    pip->INODE.i_atime = time(0L);
    pip->modified = 1;
    iput(pip);

}

int my_creat(MINODE*pip, char *name)
{
    char buf[BLKSIZE];
    int ino = ialloc(dev);
    int bno = balloc(dev);

    MINODE *mip = iget(dev,ino);
    INODE *ip = &mip->INODE;

    ip->i_mode = RMODE;		// OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = 0;     		// Size in bytes 
    ip->i_links_count = 1;	        // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
    
    for(int i = 1; i < 15; i++)
        ip->i_block[i] = 0;
 
    mip->modified = 1;            // mark minode MODIFIED
    iput(mip);                    // write INODE to disk
    enter_child(pip, ino, name);
}
