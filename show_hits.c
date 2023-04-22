// show_hits.c
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   *running;     // pointer to the currently running process

extern MINODE *cacheList;        // List of inodes that are currently in use
extern MINODE *root; // Pointer to root directory inode     

// Variables for file descriptor and command processing
extern int  dev;
extern int  requests, hits; // Variables for caching information

/**************** show_hits.c file **************/

int show_dir(MINODE *mip)  // show contents of mip DIR: same as in LAB5
{
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    // ASSUME only one data block i_block[0]
    // YOU SHOULD print i_block[0] number to see its value
    get_block(dev, mip->INODE.i_block[0], sbuf); // read the block containing directory entries

    dp = (DIR *)sbuf; // set dp to point to the first directory entry in the block
    cp = sbuf; // set the character pointer to the start of sbuf
    printf("   i_number rec_len name_len   name\n");

    while(cp < sbuf + BLKSIZE){ // loop through all directory entries in the block
        strncpy(temp, dp->name, dp->name_len); // copy the directory name into temp
        temp[dp->name_len] = 0;  // convert dp->name into a string  
        printf("%8d%8d%8u       %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

        cp += dp->rec_len;      // advance cp by rec_len
        dp = (DIR *)cp;         // pull dp to cp
    }
}

int hit_ratio()
{
    MINODE *mip = running->cwd, *temp = 0; // Get the current working directory MINODE
    int n = 0, parentIno, ino;
    char buf[BLKSIZE];
    int iArr[MAX];

    while(1){ // Traverse up the directory tree, recording the inodes in iArr
        mip->cacheCount--;
        parentIno = findino(mip, &ino); // Find the inode number and parent inode number of the current MINODE
        iArr[n++] = mip->ino;
        mip = iget(dev, parentIno); // Get the MINODE of the parent directory
        iput(mip); // Release the current MINODE
        if (mip == root) // If the current MINODE is the root directory, break
            break;
    }
    iArr[n++] = root->ino; // Add the inode number of the root directory to iArr
  
    while (mip=dequeue(&cacheList)){ // pull each minode from cacheList
        enqueue(&temp, mip); // insert each minode into temp 
    }
    cacheList = temp; // reset cacheList to node with min cachecount
    
    mip = cacheList;
    hits = requests = 0;
    printf("cacheList=");
    while(mip){
        printf("c%d[%d %d]s%d->", mip->cacheCount, mip->dev, mip->ino, mip->shareCount);
        for(int i = 0; i < n; i++){ // Check if the current MINODE's inode number is in iArr
            if (mip->ino == iArr[i]){ // If it is, add the current MINODE's cache count to hits
                hits +=mip->cacheCount;
                break;
            }
        }
        requests +=mip->cacheCount; // Add the current MINODE's cache count to requests
        mip = mip->next;
    }
    black();
    printf("NULL\n");
    white();
    printf("requests=%d hits=%d hit_ratio=%d%%\n", requests, hits, (100 * hits) / requests);
}