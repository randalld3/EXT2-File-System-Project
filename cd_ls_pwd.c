// cd_ls_pwd.c file
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   proc[NPROC];
extern PROC   *running;

extern MINODE minode[NMINODE];   // minodes
extern MINODE *freeList;         // free minodes list
extern MINODE *cacheList;        // cacheCount minodes list

extern MINODE *root;

extern OFT    oft[NOFT];

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

extern int ninodes, nblocks;
extern int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits;

int cd()
{
    printf("cd: Under Construction\n");
    MINODE* mip = path2inode(pathname);
    if (mip){
        iput(running->cwd);
        running->cwd = mip;
        printf("chdir success\n");
        return mip;
    }
    printf("Error: unable to locate dir\n");
    return 0;
    // write YOUR code for cd
}

int ls_file(MINODE *mip, char *fname)
{
    printf("ls_file: under construction\n");
    char linkname[MAX];
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";
    if(mip){
        printf("mip present\n");
    }

    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0)
    {
        printf("can't stat %s\n", fname);
        return;
    }

    printf("mode: %x", mip->INODE.i_mode);
    if ((mip->INODE.i_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((mip->INODE.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((mip->INODE.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
        
    printf("ls_file: test 1\n");
    for (i = 8; i >= 0; i--)
    {
        if (mip->INODE.i_mode & (1 << i))
            printf("%c", t1[i]); // print r|w|x 
        else
            printf("%c", t2[i]); // or print -
    }
    printf("%4d ", mip->INODE.i_links_count); // link count
    printf("%4d ", mip->INODE.i_gid);   // gid
    printf("%4d ", mip->INODE.i_uid);   // uid
    printf("%8ld ", mip->INODE.i_size);  // file size

    strcpy(ftime, ctime(mip->INODE.i_mtime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;        // kill \n at end
    printf("%s ", ftime);                
    // print name
    printf("%s", fname); // print file basename
    // print -> linkname if symbolic file
    if ((mip->INODE.i_mode & 0xF000) == 0xA000){
        // use readlink() to read linkname
        readlink(fname, linkname, MAX);
        printf(" -> %s", linkname); // print linked name }
    }
    putchar('\n');
}
  
int ls_dir(MINODE *pip)
{
    char sbuf[BLKSIZE], name[MAX];
    DIR  *dp;
    char *cp;
    MINODE* mip;
  
    printf("simple ls_dir()\n");   

    get_block(dev, pip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while (cp < sbuf + BLKSIZE){
        strncpy(name, dp->name, dp->name_len);
        name[dp->name_len] = 0;
        printf("ls_dir: next file for ls = %s\n", name);
        printf("getting: %d\n", dp->inode);
        mip = iget(dev, dp->inode); // get current file minode
        ls_file(mip, name);
        iput(mip);

        cp += dp->rec_len;
        dp = cp;
    }
    putchar('\n');
}

int ls()
{
    MINODE *mip = running->cwd;

    ls_dir(mip);
    iput(mip);
}

int pwd()
{
    MINODE *wd = running->cwd;
    if (wd == root)
        printf("/\n");
    else
        rpwd(wd);
}

rpwd(MINODE *wd)
{
    MINODE *pip;
    char buf[BLKSIZE], myname[256];   // local myname string
    int ino, parentino;

    if (wd==root)
        return;



    parentino = findino(wd, &ino);
    pip = iget(dev, parentino);

    get_block(dev, wd->INODE.i_block[0], buf);

    findmyname(pip, ino, myname);

    rpwd(pip);

    iput(pip);

    printf("/%s", myname);

}


