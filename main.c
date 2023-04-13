// main.c
#include "type.h"

/********** globals **************/
PROC   proc[NPROC];
PROC   *running;

MINODE minode[NMINODE];   // in memory INODES
MINODE *freeList;         // free minodes list
MINODE *cacheList;        // cached minodes list

MINODE *root, *reccwd;             // root minode pointer

OFT    oft[NOFT];         // for level-2 only

char gline[256];          // global line hold token strings of pathname
char *name[64];           // token string pointers
int  n;                   // number of token strings                    

int ninodes, nblocks;     // ninodes, nblocks from SUPER block
int bmap, imap, inode_start, iblk;  // bitmap, inodes block numbers

int  fd, dev;
char cmd[16], pathname[128], parameter[128], absPath[128];
int  requests, hits;

// start up files
#include "alloc.c"
#include "dalloc.c"
#include "util.c"
#include "cd_ls_pwd.c"
#include "rmdir.c"
#include "mkdir_creat.c"
#include "link_unlink.c"
#include "chmod_stat.c"

int init()
{
    int i, j;

    for (i=0; i<NMINODE; i++){  // initialize minodes into a freeList
        MINODE *mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->id = i;
        mip->next = &minode[i+1];
    }

    minode[NMINODE-1].next = 0;
    freeList = &minode[0];       // set the head of the free minodes list

    cacheList = 0;               // initialize the cache list to empty

    for (i=0; i<NOFT; i++)
        oft[i].shareCount = 0;     // set all open file table entries as free

    for (i=0; i<NPROC; i++){     // initialize processes
        PROC *p = &proc[i];    
        p->uid = p->gid = i;      // set the user and group id to process id
        p->pid = i+1;             // set process id as 1, 2, ..., NPROC

        for (j=0; j<NFD; j++)
            p->fd[j] = 0;           // set open file descriptors as 0
    }

    running = &proc[0];          // set P1 as the running process
    requests = hits = 0;         // initialize counters for minodes cache hit ratio
}

char *disk = "diskimage";

int main(int argc, char *argv[ ]) 
{
    char line[128];
    char buf[BLKSIZE];

    fd = dev = open(disk, O_RDWR);  // open the disk file
    printf("dev = %d\n", dev);  
    if (dev < 0){
        printf("Failed to open %s \n", disk); // print an error message if the disk file is not opened
        exit (1);
    }

    // get super block of dev
    get_block(dev, 1, buf); // read block 1 from disk into buf
    SUPER *sp = (SUPER *)buf;  // cast buf as a SUPER struct pointer to access the super block

    printf("check: superblock magic=%x  ", sp->s_magic);
    if (sp->s_magic != 0xEF53){ // check if it's an EXT2 file system by verifying its magic number
        printf("magic = %x it's not an ext2 file system\n", sp->s_magic); // Not ext2 system
        exit(1);
    }
    printf("OK\n");  // Is ext2 file system

    // Print the name of the working directory, preceded by a slash
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;
    

    get_block(dev, 2, buf);  // read block 2 from disk into buf
    GD *gp = (GD *)buf;  // cast buf as a GD struct pointer to access the group descriptor
    bmap = gp->bg_block_bitmap;  // set bmap to the value of the block bitmap block number
    imap = gp->bg_inode_bitmap;  // set imap to the value of the inode bitmap block number
    iblk = inode_start = gp->bg_inode_table;  // set iblk and inode_start to the value of the inode table block number
  
    init();  // call init() function to initialize some data structures
    root = iget(dev, 2);  // get the inode of the root directory (inode number 2)
    
    running->cwd = iget(dev, 2);  // set the current working directory of the running process to root
    printf("ninodes=%d  nblocks=%d inode_size=%d\n", ninodes, nblocks, sizeof(running->cwd->INODE));
    printf("inodes_per_block = %d ifactor=%d\n", BLKSIZE / sizeof(running->cwd->INODE), 
                                                 sizeof(running->cwd->INODE) / 128);
    printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, inode_start);  // print block numbers of bitmaps and inode table
    printf("mount root\n");
    printf("creating P%d as running process\n", running->pid);
    printf("root shareCount=%d\n", root->shareCount);

    while(1){
        printf("P%d running\n", running->pid); // print the process ID of the running process
        pathname[0] = parameter[0] = 0; // initialize the input strings to empty
        bzero(absPath, sizeof(absPath));
        bzero(pathname, sizeof(pathname));
        bzero(parameter, sizeof(parameter));

        printf("enter command [cd|ls|pwd|mkdir|creat|rmdir|link|unlink|symlink|chmod |exit] : "); // prompt the user to enter a command
        fgets(line, 128, stdin); // read a line from the user
        line[strlen(line)-1] = 0;    // remove the newline character from the end of the line

        if (line[0]==0)
            continue; // if the user enters nothing, continue prompting for a command
      
        sscanf(line, "%s %s %64c", cmd, pathname, parameter); // parse the command, pathname, and parameter from the input line
      
        if (strcmp(cmd, "ls")==0) // list files
            ls();
        if (strcmp(cmd, "cd")==0) // change directory
            cd();
        if (strcmp(cmd, "pwd")==0) // print working directory
            pwd();

     
        if (strcmp(cmd, "show")==0) // show directoy of current running program
            show_dir(running->cwd);
        if (strcmp(cmd, "hits")==0) //
            hit_ratio();
        if (strcmp(cmd, "exit")==0) // exit the program
            quit();

        if (strcmp(cmd, "mkdir")==0)
            make_dir();
        if (strcmp(cmd, "creat")==0)
            creat_file();
        if (strcmp(cmd, "rmdir")==0)
            rmdir();

        if (strcmp(cmd, "link")==0)
            link();
        if (strcmp(cmd, "unlink")==0)
            unlink();
        if (strcmp(cmd, "symlink")==0)
            symlink();

        if (strcmp(cmd, "chmod")==0)
            do_chmod();
    }
}

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
  
    while (mip=dequeue(&cacheList)){
        enqueue(&temp, mip);
    }
    cacheList = temp;
    
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
    printf("NULL\n");
    printf("requests=%d hits=%d hit_ratio=%d%%\n", requests, hits, (100 * hits) / requests);
}

int quit()
{
    MINODE *mip = cacheList;   // Start at the beginning of the cacheList
   
    while(mip != 0){   // Traverse the cacheList
        if (mip->shareCount){  // If the MINODE has been shared but not modified, write it back to disk
            mip->shareCount = 1;
            iput(mip); // write INODE back if modified
        }
        mip = mip->next; // Move on to the next MINODE in the list
    }
    exit(0); // Exit program
}

