//rmdir.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *root, *reccwd; // Pointer to root directory inode         

// Variables for file descriptor and command processing
extern int  dev;
extern char pathname[128], parameter[128], absPath[128];

/**************** rmdir.c file **************/

int rmdir()
{
    MINODE *mip, *pip;
    DIR *dp;
    char *cp, *buf, *childp;
    int pino;
    char child[128];

    if (!pathname[0]){ // need dir to remove
        red();
        printf("error : no pathname specified\n");
        white();
        return -1;
    }

    if (pathname[0] != '/'){ // process for building absolute pathname
        reccwd = running->cwd; // saves running->cwd, which will be modified in recAbsPath
        recAbsPath(running->cwd);
        running->cwd = reccwd;
        strcat(absPath, pathname);
        strcpy(pathname, absPath);
    }

    strcpy(child, pathname); // so we don't destroy pathname
    childp = basename(child); // name of dir to remove
    mip = path2inode(pathname); 

    if (!mip){ // dir must exist
        red();
        printf("error : ino at %s not found\n", pathname);
        white();
        return -1;
    }
    if (!S_ISDIR(mip->INODE.i_mode)){ // must remove only dir
        red();
        printf("cannot rmdir : %s is not dir\n", pathname);
        white();
        iput(mip);
        return -1;
    }
    if (mip->shareCount > 1){ // cannot remove dir with users in it
        red();
        printf("cannot rmdir : %s is busy\n", pathname);
        white();
        iput(mip);
        return -1;
    }
        
    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR *)buf; // parent directory
    cp = buf;
    cp += dp->rec_len; // advance cp to current directory
    dp = (DIR *)cp;
    cp += dp->rec_len;
    if (cp < buf + BLKSIZE){ // only remove empty directory, there should not be any more entries
        red();
        printf("cannot rmdir : %s is not empty\n", pathname);
        white();
        iput(mip);
        return -1;
    }
    pino = dp->inode;
    pip = iget(dev, pino); // get parent minode

    for (int i=0; i<12; i++){ // dallocate all blocks
        if (mip->INODE.i_block[i]==0)
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip); // (which clears mip->sharefCount to 0, still in cache);
    rm_child(pip, childp); // remove child from parent
    pip->INODE.i_links_count--; // decrement parent link count
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0L); // change parent access/modified time
    pip->modified = 1; // mark parent dirty
    iput(pip); // write parent back to disk

    return 0;
}

int rm_child(MINODE *parent, char *name)
{
    DIR *prev, *dp;
    char *cp;
    char temp[128], buf[BLKSIZE];
    for(int i = 0; i < 12; i++){ // dallocate direct blocks
        if (parent->INODE.i_block[i] == 0)
            break;

        get_block(dev, parent->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        prev = 0;
        while (cp < buf + BLKSIZE){ 
            strncpy(temp, dp->name, dp->name_len); // get name of directory entry
            temp[dp->name_len] = 0; // append null character

            if (strcmp(temp, name)==0){ // we found the file we're looking for
                if (prev){ // last entry
                    dp->inode = 0;
                    prev->rec_len += dp->rec_len;
                    put_block(dev, parent->INODE.i_block[i], buf);
                }
                else if (cp == buf && dp->rec_len == BLKSIZE){ // first and only entry
                    bdalloc(dev, parent->INODE.i_block[i]);
                    parent->INODE.i_size -= BLKSIZE;

                    while(parent->INODE.i_block[i+1] != 0 && i+1 < 12){  // deallocate all blocks
                        get_block(dev, parent->INODE.i_block[++i], buf);
                        put_block(dev, parent->INODE.i_block[i-1], buf);
                    }
                }
                else{ // middle entry, shift entries down
                    char *lastCP = buf;
                    DIR *lastDP = (DIR *)buf;
                    while(lastCP + lastDP->rec_len < buf + BLKSIZE){ // find final entry
                        lastCP += lastDP->rec_len;
                        lastDP = (DIR *)lastCP;
                    }

                    lastDP->rec_len += dp->rec_len; // add removed entry length to last entry
                    memcpy(buf, buf + dp->rec_len, BLKSIZE - dp->rec_len); // shift all entries down to removed entry
                    put_block(dev, parent->INODE.i_block[i], buf); // write back to disk
                }
                parent->modified = 1; // mark dirty
                put_block(dev, parent->INODE.i_block[i], buf); // write back to disk
            }

            prev = dp;
            cp += dp->rec_len; // advance through entries
            dp = (DIR *)cp;
        }
    }
}