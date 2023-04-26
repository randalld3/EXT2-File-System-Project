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

int ninodes, nblocks, ipb, ifactor;     // ninodes, nblocks from SUPER block, inodes per block, ifactor = inode size / 128
int bmap, imap, inode_start, iblk;  // bitmap, inodes block numbers, inode start block

int  fd, dev;
char cmd[16], pathname[128], parameter[128], absPath[128];
int  requests, hits;

/**************** main.c file **************/

// start up files
#include "alloc.c"
#include "dalloc.c"
#include "util.c"
#include "cd_ls_pwd.c"
#include "show_hits.c"
#include "rmdir.c"
#include "mkdir_creat.c"
#include "link_unlink.c"
#include "chmod_stat.c"
#include "open_close.c"
#include "read.c"
#include "write.c"
#include "cat_cp.c"
#include "style.c"
#include "head_tail.c"

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

char *disk = "disk2";

int main(int argc, char *argv[ ]) 
{
    char line[128], buf[BLKSIZE];
    int fd, gd;

    show_tux();
    
    dev = open(disk, O_RDWR);  // open the disk file
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
    int isize = sp->s_inode_size;
    ipb = BLKSIZE / isize;
    ifactor = isize / sizeof(INODE);

    get_block(dev, 2, buf);  // read block 2 from disk into buf
    GD *gp = (GD *)buf;  // cast buf as a GD struct pointer to access the group descriptor
    bmap = gp->bg_block_bitmap;  // set bmap to the value of the block bitmap block number
    imap = gp->bg_inode_bitmap;  // set imap to the value of the inode bitmap block number
    iblk = inode_start = gp->bg_inode_table;  // set iblk and inode_start to the value of the inode table block number
  
    init();  // call init() function to initialize some data structures
    root = iget(dev, 2);  // get the inode of the root directory (inode number 2)
    
    running->cwd = iget(dev, 2);  // set the current working directory of the running process to root
    printf("ninodes=%d  nblocks=%d inode_size=%d\n", ninodes, nblocks, isize); // print inode information
    printf("inodes_per_block = %d ifactor=%d\n", ipb, ifactor); // print information on inodes per block
    printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, inode_start);  // print block numbers of bitmaps and inode table
    printf("mount root\n"); 
    printf("creating P%d as running process\n", running->pid); //print running process
    printf("root shareCount=%d\n", root->shareCount); //total number of procs that are using shareCount

    while(1){
        printf("P%d running\n", running->pid); // print the process ID of the running process
        pathname[0] = parameter[0] = 0; // initialize the input strings to empty
        bzero(absPath, sizeof(absPath)); // zero out absPath
        bzero(pathname, sizeof(pathname)); // zero out pathname
        bzero(parameter, sizeof(parameter)); // zero out parameter

        green();
        printf("enter command [cd|ls|pwd|mkdir|creat|rmdir|link|unlink|symlink|chmod|"
        "open|close|dup|dup2|pfd|cat|cp|mv|head|tail |exit] : "); // prompt the user to enter a command
        fgets(line, 128, stdin); // read a line from the user
        line[strlen(line)-1] = 0;    // remove the newline character from the end of the line
        white();
        if (line[0]==0)
            continue; // if the user enters nothing, continue prompting for a command
      
        sscanf(line, "%s %s %64c", cmd, pathname, parameter); // parse the command, pathname, and parameter from the input line
      
        do_command();
    }
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

